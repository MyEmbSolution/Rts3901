#include <stdio.h>

#define SIZE 0x11000

char golden[16] __attribute__ ((aligned (8)))=
{'0', '1', '2', '3', '4', '5', '6', '7',
 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};


char testdata[SIZE] __attribute__ ((aligned (8)));

int
main (int argc, void ** argv)
{
  unsigned short *utest16 = NULL;
  unsigned long *utest32 = NULL;
  short *stest16 = NULL;
  short sdata16;
  unsigned long i, j;

  /* Initialize test data */
  for (i = 0; i < SIZE; i += 8)
    {
      for (j = 0; j < 8; j++)
        testdata[i+j] = golden[j];
    }

  /* Test unalignment access */
  printf ("\ntest lhu:\n");
  utest16 = (unsigned short *)&testdata[1];
  printf("    addr -- 0x%x, value -- 0x%x\n", utest16, *utest16);

  printf ("\ntest lh/sh:\n");
  stest16 = (short *)&testdata[9];
  printf("    origin: addr -- 0x%x, value -- 0x%x\n", stest16, *stest16);
  *stest16 = -200;
  sdata16 = *stest16;
  printf("    write -200: addr -- 0x%x, value -- %d\n", stest16, sdata16);

  printf ("\ntest lw/sw:\n");
  utest32 = (unsigned long *)&testdata[17];
  printf("    origin: addr --0x%x, value -- 0x%x\n", utest32, *utest32);
  *utest32 = 0x12345555;
  printf("    write 0x12345555: addr --0x%x, value -- 0x%x\n", utest32, *utest32);

  printf ("\ntest lw/sw 2:\n");
  utest32 = (unsigned long *)&testdata[0x10002];
  printf("    origin: addr --0x%x, value -- 0x%x\n", utest32, *utest32);
  *utest32 = 0x1a1b1c1d;
  printf("    write 0x1a1b1c1d: addr --0x%x, value -- 0x%x\n", utest32, *utest32);

  printf("\nfinish test!\n");
  return 0;
}
