#include <sys/types.h>
#include <sys/sem.h>

#define LOG_SHM_KEY ftok(".", 'c')

int
main (int argc, char **argv)
{
  key_t semkey;
  int shmid;

  semkey = LOG_SHM_KEY;
  if (semkey == (key_t) - 1)
    perror ("IPC error: ftok");

  printf ("key=%d\n", semkey);

  shmid = shmget(semkey, 32000, IPC_CREAT|IPC_EXCL|600); 
  //shmid = shmget (semkey, 0, 0);
  if (shmid == -1)
    {
      shmid = shmget (semkey, 32000, IPC_CREAT);
      if (shmid == -1)
        perror ("shmget error: ftok");
    }

  return 0;
}
