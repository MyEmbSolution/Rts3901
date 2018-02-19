#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <sched.h>

/* Test code for 10.c */

int fd;


void run_on_cpu(int cpu)
{
          cpu_set_t mask;
                  CPU_ZERO(&mask);
                          CPU_SET(cpu,&mask);
                                  sched_setaffinity(0,&mask);
}
void do_action(int cpu)
{
    char buf[100];
    int i;

    run_on_cpu(cpu);
    if(cpu == 0)
         write(fd, buf, sizeof(buf));
    else  read(fd, buf, sizeof(buf));
}

main()
{
    int status, retval, pid;
    char buf[100];

    fd = open("foo", O_RDWR);
    assert(fd >= 0);

    if((pid=fork()) == 0) 
        do_action(0);   
    else {
        do_action(1);
        wait(&status);
    }
}
