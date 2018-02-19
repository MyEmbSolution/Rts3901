#include <stdio.h>
#include <math.h>

int main(int argc, char **argv)
{
   float fa = 1.36;
   double da = 1.36;

   printf("pow = %f\n", pow(2.0, fa));
   printf("pow = %f\n", pow(2.0, da));

   printf("log10 = %f\n", log10(da));
   printf("floor = %f\n", floor(da));
   printf("ceil = %f\n", ceil(da));
   return 0;
}
