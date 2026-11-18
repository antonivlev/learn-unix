/*

:add Wash dishes
:add Do something

:list
===>
1. Wash dishes
2. Do something

:done 1

:list
===>
1. Do something

*/
#include <stdio.h>

#define N 20

int main()
{
  int c, i;

  i = 0;

  while ((c = getchar()) != EOF)
  {
    if (c == ' ' && i >= N)
    {
      putchar('\n');
      i = 0;
      continue;
    }

    putchar(c);
    ++i;
  }
}