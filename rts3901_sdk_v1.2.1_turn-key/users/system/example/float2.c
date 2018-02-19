#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *str = "1000kbit";
    char *p = NULL;
    double num;
    num = strtod(str, &p);
    printf("num=%lf\n", num);
    printf("sufix=%s\n", p);
    return 0;
}


