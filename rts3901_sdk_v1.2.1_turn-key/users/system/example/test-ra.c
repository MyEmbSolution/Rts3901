#include <stdio.h>

int main(int argc, char **argv)
{
  register int val __asm__ ("ra");

  printf("%p\n", val);
}
