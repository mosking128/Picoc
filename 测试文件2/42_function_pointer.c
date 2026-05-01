#include <stdio.h>

int fred(int p)
{
    printf("yo %d\n", p);
    return 42;
}

int main()
{
    printf("%d\n", fred(24));  
    return 0;
}