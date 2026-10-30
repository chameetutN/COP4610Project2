#include <unistd.h>
#include <sys/syscall.h>

int main(void)
{
	syscall(SYS_getpid);
	syscall(SYS_getppid);
	syscall(SYS_getuid);
	syscall(SYS_geteuid);
	syscall(SYS_getgid);

	return 0;
}


