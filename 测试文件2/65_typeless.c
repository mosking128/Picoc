#include "stdio.h"

void main()
{
    int x = 3.9;
    int y = 4;
    int *z = &y;
    int i;
    char msg[] = "hi there";
    printf("%d %d %d %s\n", x*2, y*2, *z, msg);
    printf("%d %d %d\n", sizeof(x), sizeof(y), sizeof(msg));
    for (i = 1; i <= 3; i++)
        printf("%d\n", i);

    /* this should fail
    { 
        int q = 5;
    }
    q = 3.14; // should say error
    */
}
