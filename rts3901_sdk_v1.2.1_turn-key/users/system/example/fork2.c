#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

int
main ()
{
  int i = 0;

  if (fork () != 0) exit (0);
  if (fork () != 0) exit (0);

  while (1)
    {
      sleep (1);
      printf ("process %d\n", i++);
    }
}
