#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/errno.h>

// System call function pointers
int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;

// Export pointers for the elevator kernel module
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

// Start the elevator
SYSCALL_DEFINE0(start_elevator)
{
	if (STUB_start_elevator != NULL)
		return STUB_start_elevator();
	return -ENOSYS;
}

// Issues a request
SYSCALL_DEFINE3(issue_request,
		int, start_floor,
		int, destination_floor,
		int, type)
{
	if (STUB_issue_request != NULL)
		return STUB_issue_request(start_floor,
					destination_floor,
					type);
	return -ENOSYS;
}

// Stops elevator
SYSCALL_DEFINE0(stop_elevator)
{
	if (STUB_stop_elevator != NULL)
		return STUB_stop_elevator();
	return -ENOSYS;
}
