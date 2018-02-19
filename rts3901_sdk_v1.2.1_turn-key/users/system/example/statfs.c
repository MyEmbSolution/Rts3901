#include <sys/vfs.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
 struct statfs fs;
 unsigned long long bfree, bsize;

 if (argc == 2)
   statfs(argv[1], &fs);
 else 
   statfs("/", &fs);

 bfree = fs.f_bfree;
 bsize = fs.f_bsize;
 printf("f_bfree = %llu\n", bfree);
 printf("f_bsize = %llu\n", bsize);
}

