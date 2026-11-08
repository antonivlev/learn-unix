#include <stdio.h>

/*
these are words
==>
these
are
words
*/

int main()
{
  int c;

  while ((c = getchar()) != EOF)
  {
    if (c == ' ')
      putchar('\n');
    else
      putchar(c);
  }
}