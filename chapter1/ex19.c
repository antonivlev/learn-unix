/*
reverse input line one at a time

while (there is a line)
  reverse it
  print it

new tools:
  printf - print array of chars terminated by '\0'
*/
#include <stdio.h>

#define BUF_SIZE 100

int readline(char buf[])
{
  int c, i, j;
  i = 0;

  while (i < 100)
  {
    c = getchar();
    buf[i] = c;
    ++i;
  }
  ++i;
  buf[i] = '\0';

  return i;
}

// when does getchar hold?
int main()
{
  int c, c2, i;
  char line[BUF_SIZE];

  while ((c = getchar()) != EOF)
  {
    i = 0;
    line[i] = c;
    while ((c2 = getchar()) != '\n')
    {
      ++i;
      line[i] = c2;
    }
    ++i;
    line[i] = '\0';

    printf("%s\n", line);
  }

  printf("done");
}