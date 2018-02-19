#include <stdio.h>

int main(int argc, char **argv)
{
  char *p;

  printf("hello\n");
  vsnprintf(p, 20, "%s", "hello");
  return 0;
}

