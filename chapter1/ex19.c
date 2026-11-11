/*
reverse input line one at a time

while (there is a line)
  reverse it
  print it

new tools:
  printf - print array of chars terminated by '\0'
*/
#include <stdio.h>

#define BUF_SIZE 10

int readline(char buf[])
{
  int c, i;
  i = 0;

  while ((c = getchar()) != EOF && c != '\n')
  {
    buf[i] = c;
    ++i;
  }
  buf[i] = '\0';

  return i;
}

void reverse(char buf[])
{
  char reversed[BUF_SIZE + 1];
  int i, nullindex;

  for (i = 0; i < BUF_SIZE; ++i)
  {
    if (buf[i] == '\0')
    {
      nullindex = i;
      break;
    }
  }

  for (i = 0; i < nullindex; ++i)
  {
    reversed[i] = buf[nullindex - 1 - i];
  }

  for (i = 0; i < nullindex; ++i)
  {
    buf[i] = reversed[i];
  }
}

// when does getchar hold?
int main()
{
  int c, n;
  char line[BUF_SIZE];

  while ((n = readline(line)) != 0)
  {
    reverse(line);
    printf("%s\n", line);
  }

  printf("done");
}