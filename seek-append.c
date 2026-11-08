#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifndef BUF_SIZE /* Allow "cc -D" to override definition */
#define BUF_SIZE 4
#endif

// now make `tee -a file` work, appending to end of file
int main(int argc, char *argv[])
{
  ssize_t numRead;
  char buf[BUF_SIZE];

  int optcode;
  // what is char *... ?
  char *filename;
  while ((optcode = getopt(argc, argv, "a:")) != -1)
  {
    switch (optcode)
    {
    case 'a': /* flag */
      // optarg - global var that getopt sets
      // printf("%s\n", optarg);
      filename = optarg;
      break;
    case '?': /* unknown or missing arg */
      return 1;
    }
  }

  int openFlags = O_CREAT | O_WRONLY | O_APPEND;
  int fd = open(filename, openFlags);

  if (fd == -1)
  {
    fd = STDOUT_FILENO;
  }

  // expected to write in beginning of file. why it doesn't work
  // because O_APPEND makes every `write()` call write at the end,
  // no mattter what seek did
  lseek(fd, 0, SEEK_SET);

  while ((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) != 0)
  {
    write(fd, buf, numRead);
  }
  printf("finished");
}
