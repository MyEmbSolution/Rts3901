#include <stdlib.h>
#include <stdio.h>

int a = 0;

static void my_init(void) __attribute__ ((constructor));
static void my_init(void)
{
  printf("in ctor, seting a = 100\n");
  a = 100;
}

static void my_exit(void) __attribute__ ((destructor));
static void my_exit(void)
{
  printf("in dtor, seting a = 100\n");
  a = 100;
}

int main(int argc, char **argv)
{
   //__do_global_dtors_aux();

   printf("ctor a=%d\n", a);
   return 0;
}
 
