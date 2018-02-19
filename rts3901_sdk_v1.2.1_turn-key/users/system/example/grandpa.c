#include <stdio.h>
#include <stdlib.h>

int
grandson(void)
{
  int i = 0;

  while (i < 10)
    {
      sleep (1);
      printf ("I am grandson %d\n", i++);
    }

  return 0;
}

int main(int argc, char **argv)
{
	int i = 0;

	printf ("I am grandpa\n");

	if (fork () == 0) { //son
		if (fork () == 0) {                       //grandson
			sleep (1);
			grandson();
			exit (0);
		}
		else
			exit (0);
	} else {
		wait (0);
		sleep (1);
	}

	while (i < 10) {
		sleep (1);
		printf ("I am grandpa %d\n", i++);
	}
}
