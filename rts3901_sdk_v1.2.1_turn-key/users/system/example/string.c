#include <string.h>
#include <stdio.h>


int main(int argc, char **argv)
{
  char p[32];
  char q[32];

  strcpy(p, "123456789");
  strcpy(q, "987654321");
 
  printf("p before memset = %s\n", p);
  memset(p+4, 0, 9);

  printf("p after  memset = %s\n", p);

  memcpy(p+3, q, 9);
  printf("p after  memcpy = %s\n", p);
}
