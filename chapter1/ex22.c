/* ******

software simulation

Living Figma with simulated data + ability to go to any state/screen
PRs can map to changes on the Figma

* ******/
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