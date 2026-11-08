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

// when does getchar hold?
int main()
{
  int c, n;
  char line[BUF_SIZE];

  while ((n = readline(line)) != 0)
  {
    printf("%s\n", line);
  }

  printf("done");
}