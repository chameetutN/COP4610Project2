// SPDX-License-Identifier: GPL-2.0
/*
 * elevator - a pet elevator scheduler.
 *
 * Hooks the start_elevator / issue_request / stop_elevator system calls
 * (548-550) through the STUB_* pointers exported by the kernel, runs the
 * elevator in a kthread, and reports its state through /proc/elevator.
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "../list/list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Miles Burkart");
MODULE_DESCRIPTION("Pet elevator scheduler");

#define ENTRY_NAME "elevator"
#define PERMS 0444

#define MIN_FLOOR 1
#define MAX_FLOOR 5
#define NUM_FLOORS (MAX_FLOOR - MIN_FLOOR + 1)
#define MAX_PETS 5
#define MAX_WEIGHT 50

#define MOVE_DELAY_MS 2000
#define LOAD_DELAY_MS 1000
#define IDLE_POLL_MS 100

/* Hooks defined in the kernel's syscalls/syscalls.c */
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum pet_type { CHIHUAHUA, PUG, PUGHUAHUA, DACHSHUND, NUM_PET_TYPES };

static const int pet_weight[NUM_PET_TYPES] = { 3, 14, 10, 16 };
static const char pet_char[NUM_PET_TYPES] = { 'C', 'P', 'H', 'D' };

enum elevator_state { OFFLINE, IDLE, LOADING, UP, DOWN };

static const char *const state_name[] = {
	[OFFLINE] = "OFFLINE",
	[IDLE] = "IDLE",
	[LOADING] = "LOADING",
	[UP] = "UP",
	[DOWN] = "DOWN",
};

struct pet {
	int type;
	int start_floor;
	int dest_floor;
};

struct elevator {
	enum elevator_state state;
	bool stopping;		/* stop_elevator() called, still unloading */
	int current_floor;
	int direction;		/* +1 up, -1 down */
	int load;		/* lbs */
	List pets;		/* struct pet *, pets on board */
};

struct floor {
	List waiting;		/* struct pet *, FIFO queue */
};

static struct elevator elevator;
static struct floor floors[NUM_FLOORS];	/* floors[0] is floor 1 */
static int total_waiting;
static int total_serviced;

static DEFINE_MUTEX(elevator_lock);
static struct task_struct *elevator_thread;
static struct proc_dir_entry *proc_entry;

static struct floor *floor_at(int floor_num)
{
	return &floors[floor_num - MIN_FLOOR];
}

/* ---------- scheduling helpers (caller holds elevator_lock) ---------- */

static bool pet_fits(const struct pet *pet)
{
	return elevator.pets.length < MAX_PETS &&
	       elevator.load + pet_weight[pet->type] <= MAX_WEIGHT;
}

static bool anyone_headed_to(int floor_num)
{
	ListNode *node;

	list_foreach(node, &elevator.pets)
		if (((struct pet *)node->value)->dest_floor == floor_num)
			return true;
	return false;
}

static bool anyone_to_unload(void)
{
	return anyone_headed_to(elevator.current_floor);
}

/* Pets board strictly FIFO, so only the front of the queue matters. */
static bool anyone_to_load(void)
{
	struct pet *front = list_peekfront(&floor_at(elevator.current_floor)->waiting);

	return !elevator.stopping && front && pet_fits(front);
}

static bool is_target(int floor_num)
{
	return anyone_headed_to(floor_num) ||
	       (!elevator.stopping && floor_at(floor_num)->waiting.length > 0);
}

static bool target_in_direction(int dir)
{
	int f;

	for (f = elevator.current_floor + dir;
	     f >= MIN_FLOOR && f <= MAX_FLOOR; f += dir)
		if (is_target(f))
			return true;
	return false;
}

/*
 * LOOK scheduling: keep moving the same way while there is something to do
 * in that direction, otherwise turn around. Returns 0 if there is nothing
 * to do anywhere.
 */
static int choose_direction(void)
{
	if (target_in_direction(elevator.direction))
		return elevator.direction;
	if (target_in_direction(-elevator.direction))
		return -elevator.direction;
	return 0;
}

static void unload_and_load(void)
{
	struct floor *f = floor_at(elevator.current_floor);
	ListNode *prev = NULL, *node = elevator.pets.first;
	struct pet *pet;

	while (node) {
		pet = node->value;
		if (pet->dest_floor != elevator.current_floor) {
			prev = node;
			node = node->next;
			continue;
		}
		node = node->next;
		list_remove_next(&elevator.pets, prev);
		elevator.load -= pet_weight[pet->type];
		total_serviced++;
		kfree(pet);
	}

	if (elevator.stopping)
		return;

	while ((pet = list_peekfront(&f->waiting))) {
		if (!pet_fits(pet))
			break;	/* FIFO: nobody may cut in line */
		list_transfer_front(&elevator.pets, &f->waiting);
		total_waiting--;
		elevator.load += pet_weight[pet->type];
	}
}

/* ---------- elevator thread ---------- */

static int elevator_run(void *data)
{
	int dir;

	while (!kthread_should_stop()) {
		mutex_lock(&elevator_lock);

		if (elevator.state == OFFLINE) {
			mutex_unlock(&elevator_lock);
			msleep(IDLE_POLL_MS);
			continue;
		}

		if (elevator.stopping && elevator.pets.length == 0) {
			elevator.state = OFFLINE;
			elevator.stopping = false;
			mutex_unlock(&elevator_lock);
			continue;
		}

		if (anyone_to_unload() || anyone_to_load()) {
			elevator.state = LOADING;
			mutex_unlock(&elevator_lock);
			msleep(LOAD_DELAY_MS);

			mutex_lock(&elevator_lock);
			unload_and_load();
			mutex_unlock(&elevator_lock);
			continue;
		}

		dir = choose_direction();
		if (dir == 0) {
			elevator.state = IDLE;
			mutex_unlock(&elevator_lock);
			msleep(IDLE_POLL_MS);
			continue;
		}

		elevator.direction = dir;
		elevator.state = dir > 0 ? UP : DOWN;
		mutex_unlock(&elevator_lock);
		msleep(MOVE_DELAY_MS);

		mutex_lock(&elevator_lock);
		elevator.current_floor += dir;
		mutex_unlock(&elevator_lock);
	}

	return 0;
}

/* ---------- system call implementations ---------- */

static int start_elevator(void)
{
	int ret = 0;

	mutex_lock(&elevator_lock);
	if (elevator.state != OFFLINE) {
		ret = 1;	/* already active */
	} else {
		elevator.state = IDLE;
		elevator.stopping = false;
		elevator.current_floor = MIN_FLOOR;
		elevator.direction = 1;
		elevator.load = 0;
	}
	mutex_unlock(&elevator_lock);
	return ret;
}

static int issue_request(int start_floor, int destination_floor, int type)
{
	struct pet *pet;

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR ||
	    destination_floor < MIN_FLOOR || destination_floor > MAX_FLOOR ||
	    start_floor == destination_floor ||
	    type < 0 || type >= NUM_PET_TYPES)
		return 1;

	pet = kmalloc(sizeof(*pet), GFP_KERNEL);
	if (!pet)
		return -ENOMEM;
	pet->type = type;
	pet->start_floor = start_floor;
	pet->dest_floor = destination_floor;

	mutex_lock(&elevator_lock);
	if (list_append(&floor_at(start_floor)->waiting, pet)) {
		mutex_unlock(&elevator_lock);
		kfree(pet);
		return -ENOMEM;
	}
	total_waiting++;
	mutex_unlock(&elevator_lock);
	return 0;
}

static int stop_elevator(void)
{
	int ret = 0;

	mutex_lock(&elevator_lock);
	if (elevator.stopping)
		ret = 1;	/* already deactivating */
	else if (elevator.state != OFFLINE)
		elevator.stopping = true;
	mutex_unlock(&elevator_lock);
	return ret;
}

/* ---------- /proc/elevator ---------- */

static void print_pet(struct seq_file *m, const struct pet *pet)
{
	seq_printf(m, " %c%d", pet_char[pet->type], pet->dest_floor);
}

static int elevator_show(struct seq_file *m, void *v)
{
	ListNode *node;
	int f;

	mutex_lock(&elevator_lock);

	seq_printf(m, "Elevator state: %s\n", state_name[elevator.state]);
	seq_printf(m, "Current floor: %d\n", elevator.current_floor);
	seq_printf(m, "Current load: %d lbs\n", elevator.load);
	seq_puts(m, "Elevator status:");
	list_foreach(node, &elevator.pets)
		print_pet(m, node->value);
	seq_puts(m, "\n\n");

	for (f = MAX_FLOOR; f >= MIN_FLOOR; f--) {
		seq_printf(m, "[%c] Floor %d: %d",
			   f == elevator.current_floor ? '*' : ' ',
			   f, (int)floor_at(f)->waiting.length);
		list_foreach(node, &floor_at(f)->waiting)
			print_pet(m, node->value);
		seq_putc(m, '\n');
	}

	seq_printf(m, "\nNumber of pets: %d\n", (int)elevator.pets.length);
	seq_printf(m, "Number of pets waiting: %d\n", total_waiting);
	seq_printf(m, "Number of pets serviced: %d\n", total_serviced);

	mutex_unlock(&elevator_lock);
	return 0;
}

static int elevator_open(struct inode *inode, struct file *file)
{
	return single_open(file, elevator_show, NULL);
}

static const struct proc_ops elevator_fops = {
	.proc_open = elevator_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* ---------- module init / exit ---------- */

static void free_pet(void *pet)
{
	kfree(pet);
}

static int __init elevator_init(void)
{
	int i;

	elevator.state = OFFLINE;
	elevator.current_floor = MIN_FLOOR;
	elevator.direction = 1;
	list_init(&elevator.pets);
	for (i = 0; i < NUM_FLOORS; i++)
		list_init(&floors[i].waiting);

	proc_entry = proc_create(ENTRY_NAME, PERMS, NULL, &elevator_fops);
	if (!proc_entry)
		return -ENOMEM;

	elevator_thread = kthread_run(elevator_run, NULL, "elevator");
	if (IS_ERR(elevator_thread)) {
		proc_remove(proc_entry);
		return PTR_ERR(elevator_thread);
	}

	STUB_start_elevator = start_elevator;
	STUB_issue_request = issue_request;
	STUB_stop_elevator = stop_elevator;

	pr_info("elevator: loaded\n");
	return 0;
}

static void __exit elevator_exit(void)
{
	int i;

	STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

	kthread_stop(elevator_thread);
	proc_remove(proc_entry);

	list_clear(&elevator.pets, free_pet);
	for (i = 0; i < NUM_FLOORS; i++)
		list_clear(&floors[i].waiting, free_pet);

	pr_info("elevator: unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);
