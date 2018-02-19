#include <stdio.h>
#include <pthread.h>

static void
VoipFlashServerWriteCoreThread (void *ptr)
{

  printf ("threaf start: %d \n", getpid ());
  while (1)
    {
      sleep (2);
      printf (" thread %d \n", getpid ());
    }
}

int
main (int x, char *argv[])
{
  pthread_t thFlashWrite;

  pthread_create (&thFlashWrite, NULL,
                  (void *) &VoipFlashServerWriteCoreThread, (void *) NULL);

  int pid;
  while (1)
    {
      pid = fork ();
      if (pid == 0)
        {
          //sleep(20);
          printf ("==========11111111111111======\n");
          exit (1);
        }
      sleep (2);
    }
  return 0;
}
