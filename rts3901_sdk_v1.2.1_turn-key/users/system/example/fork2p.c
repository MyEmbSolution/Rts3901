#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

void *
thread_func (void *arg)
{
  char *a;
  int i = 0;
  printf("a=%p\n", a);

  while (1)
    {
      printf ("thread %d\n", i++);
      sleep (1);
    }
}

int
main ()
{
  pthread_t p_tid;
  int i = 0;

  if (fork () != 0) exit (0);
  if (fork () != 0) exit (0);

  pthread_create (&p_tid, NULL, &thread_func, NULL);

  while (1)
    {
      printf ("process %d\n", i++);
      sleep (1);
    }
}
