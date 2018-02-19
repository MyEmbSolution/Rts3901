/* shm.c: demonstrate simple shmget/shmat functionality */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>

int
main (int argc, char **argv)
{
  if (argc != 2) 
    {
      printf("usage: shm [digit]\n");
      return 0;
    }

  key_t key = atoi (argv[1]);
  int id = shmget (key, 0x1000, IPC_CREAT);
  fprintf (stderr, "id = %d\n", id);
  struct shmid_ds shmid_ds;
  register int retval = shmctl (id, IPC_STAT, &shmid_ds);
  if (retval == -1)
    {
      perror ("shmctl: shmctl failed");
      return 1;
    }
  else
    {
      fprintf (stderr, "\tshm_perm.uid = %d\n", shmid_ds.shm_perm.uid);
      fprintf (stderr, "\tshm_perm.gid = %d\n", shmid_ds.shm_perm.gid);
      fprintf (stderr, "\tshm_perm.cuid = %d\n", shmid_ds.shm_perm.cuid);
      fprintf (stderr, "\tshm_perm.cgid = %d\n", shmid_ds.shm_perm.cgid);
      fprintf (stderr, "\tshm_perm.mode = %#o\n", shmid_ds.shm_perm.mode);
      fprintf (stderr, "\tshm_segsz = %d\n", shmid_ds.shm_segsz);
      fprintf (stderr, "\tshm_lpid = %d\n", shmid_ds.shm_lpid);
      fprintf (stderr, "\tshm_cpid = %d\n", shmid_ds.shm_cpid);
      fprintf (stderr, "\tshm_nattch = %d\n", shmid_ds.shm_nattch);
      fprintf (stderr, "\tshm_atime = %s",
               shmid_ds.shm_atime ? ctime (&shmid_ds.
                                           shm_atime) : "Not Set\n");
      fprintf (stderr, "\tshm_dtime = %s",
               shmid_ds.shm_dtime ? ctime (&shmid_ds.
                                           shm_dtime) : "Not Set\n");
      fprintf (stderr, "\tshm_ctime = %s", ctime (&shmid_ds.shm_ctime));
    }

  getc (stdin);
  fprintf (stderr, "Mapping...");
  void *myptr = shmat (id, NULL, 0);
  if (myptr == (void *) -1)
    {
      perror ("shmat: shmat failed");
      return 1;
    }
  retval = shmctl (id, IPC_STAT, &shmid_ds);
  if (retval == -1)
    {
      perror ("shmctl: shmctl failed");
      return 1;
    }
  else
    {
      fprintf (stderr, "\tshm_perm.uid = %d\n", shmid_ds.shm_perm.uid);
      fprintf (stderr, "\tshm_perm.gid = %d\n", shmid_ds.shm_perm.gid);
      fprintf (stderr, "\tshm_perm.cuid = %d\n", shmid_ds.shm_perm.cuid);
      fprintf (stderr, "\tshm_perm.cgid = %d\n", shmid_ds.shm_perm.cgid);
      fprintf (stderr, "\tshm_perm.mode = %#o\n", shmid_ds.shm_perm.mode);
      fprintf (stderr, "\tshm_segsz = %d\n", shmid_ds.shm_segsz);
      fprintf (stderr, "\tshm_lpid = %d\n", shmid_ds.shm_lpid);
      fprintf (stderr, "\tshm_cpid = %d\n", shmid_ds.shm_cpid);
      fprintf (stderr, "\tshm_nattch = %d\n", shmid_ds.shm_nattch);
      fprintf (stderr, "\tshm_atime = %s",
               shmid_ds.shm_atime ? ctime (&shmid_ds.
                                           shm_atime) : "Not Set\n");
      fprintf (stderr, "\tshm_dtime = %s",
               shmid_ds.shm_dtime ? ctime (&shmid_ds.
                                           shm_dtime) : "Not Set\n");
      fprintf (stderr, "\tshm_ctime = %s", ctime (&shmid_ds.shm_ctime));
    }
  getc (stdin);

  fprintf (stderr, "Unmapping...");
  shmdt (myptr);

  retval = shmctl (id, IPC_STAT, &shmid_ds);
  if (retval == -1)
    {
      perror ("shmctl: shmctl failed");
      return 1;
    }
  else
    {
      fprintf (stderr, "\tshm_perm.uid = %d\n", shmid_ds.shm_perm.uid);
      fprintf (stderr, "\tshm_perm.gid = %d\n", shmid_ds.shm_perm.gid);
      fprintf (stderr, "\tshm_perm.cuid = %d\n", shmid_ds.shm_perm.cuid);
      fprintf (stderr, "\tshm_perm.cgid = %d\n", shmid_ds.shm_perm.cgid);
      fprintf (stderr, "\tshm_perm.mode = %#o\n", shmid_ds.shm_perm.mode);
      fprintf (stderr, "\tshm_segsz = %d\n", shmid_ds.shm_segsz);
      fprintf (stderr, "\tshm_lpid = %d\n", shmid_ds.shm_lpid);
      fprintf (stderr, "\tshm_cpid = %d\n", shmid_ds.shm_cpid);
      fprintf (stderr, "\tshm_nattch = %d\n", shmid_ds.shm_nattch);
      fprintf (stderr, "\tshm_atime = %s",
               shmid_ds.shm_atime ? ctime (&shmid_ds.
                                           shm_atime) : "Not Set\n");
      fprintf (stderr, "\tshm_dtime = %s",
               shmid_ds.shm_dtime ? ctime (&shmid_ds.
                                           shm_dtime) : "Not Set\n");
      fprintf (stderr, "\tshm_ctime = %s", ctime (&shmid_ds.shm_ctime));
    }
  getc (stdin);

  return 0;
}
