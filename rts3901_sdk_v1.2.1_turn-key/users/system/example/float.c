#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv)
{
   float a = 1.36;
   double da = 1.36;

   printf("pow = %f\n", pow(2.0, a));
   printf("pow = %f\n", pow(2.0, da));

   printf("float a=%f\n", a);
   printf("int a =%d\n", (int)a);
   printf("isnan a = %d\n", isnan(a));
   printf("finitef a = %d\n", finitef(a));

   return 0;
}
