#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

int
grandson (void)
{
  int i = 0;

  while (i < 10)
    {
      sleep (1);
      printf ("I am grandson %d\n", i++);
    }

  return 0;
}

void *
task1 (void *arg)
{
  pid_t pid, pid1, pid2;

  pid = getpid ();
  if ((pid1 = fork ()) == 0)
    {                           //son
      if ((pid2 = fork ()) == 0)
        {                       //grandson
          sleep (1);
          grandson();
        }
      else if (pid2 > 0)
        {
          printf ("grandson pid:%d\n", pid2);
          exit (1);
        }
      else
        {
          printf ("fork 2 fail\n");
        }
    }
  else if (pid1 > 0)
    {
      printf ("son pid:%d\n", pid1);
      wait (0);
    }
  else
    {
      printf ("fork 1 fail\n");
    }

  while (1)
    {
      printf ("I am new thread!%d\n", pid);
      sleep (1);
    }
}

int
main ()
{
  pid_t pid1;
  pthread_t pid;

  pthread_create (&pid, NULL, task1, NULL);
  pid1 = getpid ();
  printf ("I am grandpa %d\n", pid1);

  while (1)
    {
      printf ("I am grandpa %d\n", pid1);
      sleep (1);
    }
}
