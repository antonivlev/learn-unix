#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_READ 20

static void errExit(const char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(void)
{
  char buffer[MAX_READ + 1];
  ssize_t numRead;

  numRead = read(STDIN_FILENO, buffer, MAX_READ);
  if (read(STDIN_FILENO, buffer, MAX_READ) == -1)
    errExit("read");

  buffer[numRead] = '\0';
  printf("The input data was: %s\n", buffer);

  return 0;
}