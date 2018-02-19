#include <stdfix.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  _Accum a;
  _Accum b;

  a = 1.0k;
  b = 1.24145k;

  printf("%.6f + %.6f = %.6f\n", (float) a, (float) b, (float) a+b);
}
