[← Back to index](README.md)

# exit (syscall 1) and exit_group (syscall 252)

Terminate the calling thread (`exit`) or the whole process (`exit_group`) with an exit status.

## What it does
`exit` ends the calling **thread** and reports an exit status to the parent. In a single-threaded assembly program that's the same as ending the program. `exit_group` ends **every thread in the process** — and it's what C's `exit()` (and `return` from `main`) actually calls. Neither syscall ever returns.

## How it works
```c
void _exit(int status);        /* raw exit / exit_group */
```

| Register | `exit` | `exit_group` |
|---|---|---|
| `eax` | 1 | 252 |
| `ebx` | exit status (0 = success) | exit status |

| Parameter | Meaning |
|---|---|
| `status` | **In.** The number your program hands to its parent as its result. Only `status & 0xff` is kept, so the parent sees 0–255: a negative value wraps (`-1` reads as 255) and `256` reads as 0. The parent gets it from `waitpid` in bits 8–15 of the status word; a shell shows it as `$?`. 0 means success, nonzero means failure. With `exit_group` it is the status of the whole process |

Return value: none — these calls never return, so there are no errors to check. Only the low 8 bits of the status are visible to the parent (via `waitpid`), so stick to 0–255: `exit(256)` reads as 0.

By convention, status 0 means success and any nonzero value means failure (libc names them `EXIT_SUCCESS` = 0 and `EXIT_FAILURE` = 1). Shells use this: `&&` runs the next command only after status 0, `||` only after a nonzero status.

## Example
```nasm
; nasm -f elf32 exit.asm && i686-linux-gnu-ld -o exit exit.o && qemu-i386-static ./exit
; run it, then check: echo $?   -> prints 42
section .text
global _start
_start:
    mov eax, 1          ; exit (use 252 for exit_group; same effect here)
    mov ebx, 42         ; exit status
    int 0x80
```

```c
#include <stdlib.h>
exit(42);   /* runs atexit handlers, flushes stdio, then exit_group(42) */
```

## Linux philosophy
Every process ends with a small integer status — that's the entire contract between a dying process and its parent, and it's what makes shell pipelines and scripts composable (`&&`, `||`, `$?`). The exit/exit_group split shows how Linux retrofitted threads onto UNIX: threads are kernel tasks, so the original per-task `exit` had to be joined by `exit_group` to kill the whole group at once.

## In The Linux Programming Interface
Ch 25 "Process Termination" (p.531). The book explains the difference between `_exit()` and library `exit()` — exit handlers, stdio buffer flushing — and why calling `exit()` in a forked child can flush buffers twice.
