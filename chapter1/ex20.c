/*
hello this is a very long line here

===>
n=10

hello this
is a very
long line
here

*/
#include <stdio.h>

#define N 10

int main()
{
  int c, i;

  i = 0;

  while ((c = getchar()) != EOF)
  {
    if (i % N == 0)
    {
      putchar('\n');
      putchar(c);
    }
    else
      putchar(c);

    ++i;
  }
}