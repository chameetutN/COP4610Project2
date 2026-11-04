// SPDX-License-Identifier: GPL-2.0
/*
 * my_timer - creates /proc/timer. Each read prints the current wall-clock
 * time (from ktime_get_real_ts64) and, after the first read, the time
 * elapsed since the previous read.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Miles Burkart");
MODULE_DESCRIPTION("Reports current time and elapsed time via /proc/timer");

#define ENTRY_NAME "timer"
#define PERMS 0444
#define PARENT NULL
#define BUF_LEN 256

static struct proc_dir_entry *timer_entry;

static struct timespec64 last_time;	/* time stored at the previous read */
static bool last_time_valid;		/* false until the first read */

static char msg[BUF_LEN];
static int msg_len;
static DEFINE_MUTEX(timer_lock);

/* Build the output and update the stored time. Caller holds timer_lock. */
static void build_message(void)
{
	struct timespec64 now, elapsed;

	ktime_get_real_ts64(&now);

	msg_len = scnprintf(msg, BUF_LEN, "current time: %lld.%09ld\n",
			    (long long)now.tv_sec, now.tv_nsec);

	if (last_time_valid) {
		elapsed = timespec64_sub(now, last_time);
		msg_len += scnprintf(msg + msg_len, BUF_LEN - msg_len,
				     "elapsed time: %lld.%09ld\n",
				     (long long)elapsed.tv_sec,
				     elapsed.tv_nsec);
	}

	last_time = now;
	last_time_valid = true;
}

static ssize_t timer_read(struct file *file, char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&timer_lock);

	/*
	 * Only sample the clock at the start of a read (offset 0), so a
	 * single `cat`, which keeps reading until it gets EOF, counts
	 * as one read operation rather than several.
	 */
	if (*ppos == 0)
		build_message();

	ret = simple_read_from_buffer(ubuf, count, ppos, msg, msg_len);

	mutex_unlock(&timer_lock);
	return ret;
}

static const struct proc_ops timer_fops = {
	.proc_read = timer_read,
};

static int __init timer_init(void)
{
	timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
	if (!timer_entry) {
		pr_err("my_timer: failed to create /proc/%s\n", ENTRY_NAME);
		return -ENOMEM;
	}

	pr_info("my_timer: /proc/%s created\n", ENTRY_NAME);
	return 0;
}

static void __exit timer_exit(void)
{
	proc_remove(timer_entry);
	pr_info("my_timer: /proc/%s removed\n", ENTRY_NAME);
}

module_init(timer_init);
module_exit(timer_exit);
