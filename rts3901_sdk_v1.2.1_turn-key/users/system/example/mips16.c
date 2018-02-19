#include <stdio.h>

void __attribute__((nomips16)) test_mips32 (void)
{
   printf("This is a mips1 function\n");
   return;
}

void __attribute__((mips16)) test_mips16 (void)
{
   printf("This is a mips16 function\n");
   return;
}

int main(int argc, char **argv)
{
   test_mips16();
   test_mips32();
   return 0;
}

