[← Back to index](README.md)

# getpid (syscall 20)

Return the process ID of the calling process.

## What it does
Every process has a unique positive integer, its PID. `getpid` simply returns yours. Related calls: `getppid` (64) returns the **parent's** PID, and `gettid` (224) returns the calling **thread's** ID (in a single-threaded process, `gettid == getpid`). These calls take no arguments and cannot fail.

## How it works
```c
pid_t getpid(void);
pid_t getppid(void);
pid_t gettid(void);
```

| Register | `getpid` | `getppid` | `gettid` |
|---|---|---|---|
| `eax` | 20 | 64 | 224 |
| `ebx`, `ecx`, `edx`, ... | not used | not used | not used |

**Parameters:** none. All three calls take no arguments: put the syscall number in `eax`, execute `int 0x80`, and read the answer from `eax`. Whatever is in the other registers is ignored.

Return in `eax` (always succeeds — these are among the very few syscalls with no error case, so the value is never negative):

| Call | Returns |
|---|---|
| `getpid` | your process's PID, a positive integer. Every thread in the process gets the same number (it is the TID of the process's first thread) |
| `getppid` | the PID of your parent process: the one that forked you. If the parent has already exited, you were re-parented (normally to init, PID 1), and you get that new parent's PID |
| `gettid` | the calling thread's ID. Each thread has its own; in the first (or only) thread it equals the PID |

## Example
```nasm
; nasm -f elf32 pid.asm && i686-linux-gnu-ld -o pid pid.o && qemu-i386-static ./pid
; exits with (pid & 0xff) as its status: run it, then  echo $?
section .text
global _start
_start:
    mov eax, 20         ; getpid()
    int 0x80

    mov ebx, eax        ; pass PID out as the exit status
    and ebx, 0xff       ; only 8 bits fit in an exit status
    mov eax, 1          ; exit
    int 0x80
```

```c
#include <unistd.h>
printf("pid=%d ppid=%d\n", getpid(), getppid());
```

## Linux philosophy
PIDs are how UNIX names processes: a flat namespace of small integers, handed out by the kernel, usable by anyone (`kill`, `waitpid`, `/proc/PID/`). That every process also knows its *parent's* PID reflects the strict tree structure of UNIX processes — everything descends from init (PID 1), and `getppid` lets you look one level up that tree.

## In The Linux Programming Interface
Ch 6 "Processes" (p.113), §6.1. The book explains the process tree, how PIDs are allocated and recycled after wrapping around, and the memory layout of a process.
