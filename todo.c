#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifndef BUF_SIZE /* Allow "cc -D" to override definition */
#define BUF_SIZE 100
#endif

/*
Usage:

todo

```
Welcome! All commands start with ":". Try ":help".
>
```

:help
:add Some task here
:mark id done/undone
:list


Mon 26 Oct
print issues (see comments). Try writing to stdout instead?
*/

#define WELCOME_TEXT "Welcome!\n"

int main(int argc, char *argv[])
{
  ssize_t numRead;
  char buf[BUF_SIZE] = {};

  printf(WELCOME_TEXT);
  while ((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) != 0)
  {
    printf("You typed: %s", buf);
  }
  printf("finished\n");
}
