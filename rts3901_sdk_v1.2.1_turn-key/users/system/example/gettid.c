/*
 * Realtek Semiconductor Corp.
 * 
 * Tony Wu (tonywu@realtek.com)
 */
#include <sys/syscall.h>
#include <stdio.h>

#ifndef gettid
int gettid(void)
{
	return syscall(__NR_gettid);
}
#endif

int main(int argc, char **argv)
{
	printf("The ID of this of this thread is: %d\n", gettid());
}
