/*This is a program to illustrate
*the shared memory get, shmget(),
*system call capabilities.*/

#include    <sys/types.h>
#include    <sys/ipc.h>
#include    <sys/shm.h>
#include    <errno.h>


/*Start of main C language program*/
main ()
{
  key_t key;			/*declare as long integer */
  int opperm, flags;
  int shmid, size, opperm_flags;
/*Enter the desired key*/
  printf ("Enter the desired key in hex = ");
  scanf ("%x", &key);


/*Enter the desired octal operation
permissions.*/
  printf ("\nEnter the operation\n");
  printf ("permissions in octal = ");
  scanf ("%o", &opperm);


/*Set the desired flags.*/
  printf ("\nEnter corresponding number to\n");
  printf ("set the desired flags:\n");
  printf ("No flags                  = 0\n");
  printf ("IPC_CREAT                 = 1\n");
  printf ("IPC_EXCL                  = 2\n");
  printf ("IPC_CREAT and IPC_EXCL    = 3\n");
  printf ("            Flags         = ");
/*Get the flag(s) to be set.*/
  scanf ("%d", &flags);


/*Check the values.*/
  printf ("\nkey =0x%x, opperm = 0%o, flags = %d\n", key, opperm, flags);


/*Incorporate the control fields (flags) with
the operation permissions*/
  switch (flags)
    {
    case 0:			/*No flags are to be set. */
      opperm_flags = (opperm | 0);
      break;
    case 1:			/*Set the IPC_CREAT flag. */
      opperm_flags = (opperm | IPC_CREAT);
      break;
    case 2:			/*Set the IPC_EXCL flag. */
      opperm_flags = (opperm | IPC_EXCL);
      break;
    case 3:			/*Set the IPC_CREAT and IPC_EXCL flags. */
      opperm_flags = (opperm | IPC_CREAT | IPC_EXCL);
    }

/*Get the size of the segment in bytes.*/
  printf ("\nEnter the segment");
  printf ("\nsize in bytes = ");
  scanf ("%d", &size);
/*Call the shmget system call.*/
  shmid = shmget (key, size, opperm_flags);
/*Perform the following if the call is unsuccessful.*/
  if (shmid == -1)
    {
      printf ("\nThe shmget call failed, error number = %d\n", errno);
    }
/*Return the shmid upon successful completion.*/
  else
    printf ("\nThe shmid = %d\n", shmid);
  exit (0);
}
