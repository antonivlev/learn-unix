#include <stdio.h>
#include <string.h>

char NAME[] = "anton";

// no pointer: requires to specify count in advance
char SOME_NAMES[2][10] = {"anton", "guy"};

// pointer: no count needed
char *OTHER_NAMES[] = {"bob", "dave", "ok"};

int v[] = {48, 49, 50};

// argv is an array of pointers
int main(int argc, char *argv[])
{
    int x = 23;
    // expression "*p" is an int
    int *p;
    p = &x;

    // *p = 78;

    printf("address of x:   %p\n", (void *)p);
    printf("x is %d\n", *p);

    return 0;
}
