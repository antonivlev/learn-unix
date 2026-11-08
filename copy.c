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

// what does "char *name[]" syntax mean?
// how is a list of strings represented in c?
int main(int argc, char *argv[])
{
    if (strcmp(argv[1], "--help") == 0)
    {
        printf("%s old-file new-file\n", argv[0]);
        return 1;
    }

    int inputFd, outputFd, openFlags;
    mode_t filePerms;
    ssize_t numRead;
    char buf[BUF_SIZE];

    inputFd = open(argv[1], O_RDONLY);
    // -1 means error occured. To get actual error, import `errno` and read that
    if (inputFd == -1)
    {
        printf("opening file %s", argv[1]);
        return 1;
    }

    // bitwise OR
    openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    outputFd = open(argv[2], openFlags, filePerms);
    if (outputFd == -1)
    {
        printf("opening file %s", argv[2]);
        return 1;
    }

    numRead = read(inputFd, buf, BUF_SIZE);
    while (numRead != 0)
    {
        printf("numRead: %d\n", numRead);
        write(outputFd, buf, numRead);
        numRead = read(inputFd, buf, BUF_SIZE);
    }
    printf("finished");
}
