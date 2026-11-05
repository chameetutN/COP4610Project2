# Elevator

COP4610 Project 2: system calls, kernel modules, and synchronization in the Linux kernel.

- **Part 1** traces a C program that makes exactly five more system calls than an empty program.
- **Part 2** is the `my_timer` kernel module, which reports the current and elapsed time
  through `/proc/timer`.
- **Part 3** is the `elevator` kernel module, which schedules a pet elevator using three custom
  system calls and reports its state through `/proc/elevator`.

## Group Members
- Chamee Nernginn
- Miles Burkart
- Upi Shanker

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Write `empty.c` and `part1.c`, and capture their `strace` output.
- **Assigned to**: Chamee

### Part 2: Timer Kernel Module
- **Responsibilities**: Implement `my_timer` and its `/proc/timer` entry using
  `ktime_get_real_ts64()`.
- **Assigned to**: Miles

### Part 3a: Adding System Calls
- **Responsibilities**: Add `start_elevator` (548), `issue_request` (549), and `stop_elevator`
  (550) to the kernel.
- **Assigned to**: Chamee

### Part 3b: Kernel Compilation
- **Responsibilities**: Compile and install the modified 6.16.x kernel, and document the setup.
- **Assigned to**: Chamee

### Part 3c: Threads
- **Responsibilities**: Run the elevator in a kthread that starts and stops with the module.
- **Assigned to**: Miles

### Part 3d: Linked List
- **Responsibilities**: Implement the linked list used for the elevator and floor queues.
- **Assigned to**: Miles

### Part 3e: Mutexes
- **Responsibilities**: Protect the elevator and floor state with a mutex.
- **Assigned to**: Upi

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Decide when the elevator moves, loads, and unloads pets.
- **Assigned to**: Upi

## File Listing
```
elevator/
├── Makefile
├── README.md
├── Starter Files.zip
├── part1/
│   ├── Makefile
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   └── part1.trace
├── part2/
│   ├── Makefile
│   └── src/
│       └── my_timer.c
└── part3/
    ├── Makefile
    ├── syscalls.c
    ├── testing.md
    ├── src/
    │   ├── elevator.c
    │   ├── list.c
    │   └── list.h
    └── tests/
        ├── elevator-test/
        │   ├── Makefile
        │   ├── README.md
        │   ├── consumer.c
        │   ├── producer.c
        │   └── wrappers.h
        └── system-calls-test/
            ├── Makefile
            ├── README.md
            ├── syscheck.c
            ├── test-syscalls.c
            └── test-syscalls.h
```

# How to Compile & Execute

### Requirements
- **Tools**: `gcc`, `make`, and `strace`
- **Kernel**: Linux 6.16.x with the elevator system calls installed (needed for Part 3)
- **Kernel build files**: the build tree for the running kernel, at
  `/lib/modules/$(uname -r)/build` (needed for Parts 2 and 3)

Running `make` in the repository root builds all three parts, and `make clean` removes the build
output.

## Part 1

### Compilation
```bash
cd part1
make
```
This builds the `empty` and `part1` executables in `part1/`.

### Execution
```bash
make traces
```
This runs both programs under `strace` and writes `empty.trace` and `part1.trace`.

## Part 2

### Compilation
```bash
cd part2
make
```
This builds `my_timer.ko` in `part2/`.

### Execution
```bash
sudo insmod my_timer.ko
cat /proc/timer
sudo rmmod my_timer
```
The first read prints the current time. Later reads also print the time elapsed since the
previous read.

## Part 3

### Kernel Setup
Part 3 requires a kernel that includes the system calls in `part3/syscalls.c`.

1. Copy `syscalls.c` to `syscalls/syscalls.c` in the kernel source tree and add a
   `syscalls/Makefile`.
2. Update `arch/x86/entry/syscalls/syscall_64.tbl`, `include/linux/syscalls.h`, and the top-level
   `Makefile`.
3. Build and install the kernel, then reboot:
   ```bash
   make menuconfig
   make -j$(nproc)
   sudo make modules_install
   sudo make install
   sudo reboot
   ```
4. Run `uname -r` to confirm that the new kernel is running.
5. Check that the system calls are installed:
   ```bash
   cd part3/tests/system-calls-test
   make
   sudo insmod syscheck.ko
   make run
   sudo rmmod syscheck
   ```

### Compilation
```bash
cd part3
make
```
This builds `elevator.ko` in `part3/`.

### Execution
```bash
sudo insmod elevator.ko
cd tests/elevator-test
make
./consumer --start
./producer 20
watch -n1 cat /proc/elevator
```
Stop `watch` with Ctrl+C, then stop the elevator and remove the module:
```bash
./consumer --stop
sudo rmmod elevator
```
Wait for `/proc/elevator` to show `OFFLINE` before running `rmmod`, so that the elevator can
finish unloading its pets.

## Development Log
Entries for Chamee and Miles come from the commit history. Each member should add any work that
isn't shown there.

### Chamee Nernginn

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-29 | Uploaded the starter files and README, and added the Part 1 trace files |
| 2026-11-02 | Compiled the kernel, uploaded the kernel files, and wrote the setup instructions |
| 2026-11-04 | Fixed `part3/Makefile` to build `src/list.o`, and added `part1/Makefile` |

### Miles Burkart

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-03 | Added the linked list and the `my_timer` kernel module |
| 2026-11-04 | Implemented the elevator module and fixed the list implementation |
| 2026-11-04 | Added testing instructions, moved the list into `src/`, and removed `kernelSetup/` |

### Upi Shanker

| Date       | Work Completed / Notes          |
|------------|---------------------------------|
| 2026-11-04       | Tested files and updated Readme |

## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date    | Attendees          | Topics Discussed                     | Outcomes / Decisions |
|---------|--------------------|--------------------------------------|----------------------|
| 11/1/26 | Upi, Miles, Chamee | Testing elevator and practicing demo | N/A                  |


## Considerations
- **Scheduling**: The elevator uses LOOK scheduling. It keeps moving in one direction while there
  is work ahead, then turns around.
- **Boarding**: Pets board in strict FIFO order. If the pet at the front of a floor's queue doesn't
  fit, the pets behind it wait too.
- **Stopping**: After `stop_elevator()`, the elevator stops loading and delivers the pets on board
  before going `OFFLINE`. Pets still waiting stay on their floors.
- **Requests while offline**: `issue_request()` still queues pets while the elevator is `OFFLINE`,
  and they are serviced once it starts again.
- **Timing**: The mutex is released during the 1-second load and 2-second move delays, so system
  calls and `/proc/elevator` reads don't block while the elevator waits.
