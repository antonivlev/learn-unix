[← Back to index](README.md)

# fork (syscall 2)

Duplicate the calling process, creating a nearly identical child.

## What it does
`fork` clones the current process: the child gets a copy of the parent's memory, open file descriptors, and registers. Both processes continue from the instruction right after the `int 0x80` — the only difference is the return value. The kernel doesn't actually copy memory up front; pages are shared **copy-on-write** and only duplicated when one side writes to them, so fork is cheap.

## How it works
```c
pid_t fork(void);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 2 |
| `ebx`, `ecx`, `edx`, ... | — | not used: `fork` takes no arguments and ignores the other registers |

Return in `eax` — the call returns twice, once in each process, with a different value:

| Where | `eax` | Meaning |
|---|---|---|
| parent | child's PID (> 0) | the new process's ID; keep it so you can `waitpid` for the child or `kill` it |
| child | 0 | "you are the child"; the child can find its own PID with `getpid` and the parent's with `getppid` |
| parent, on failure | negative `-errno` | no child was created, so only the parent sees this |

Common errors: `-EAGAIN` (-11) — process limit reached; `-ENOMEM` (-12) — out of kernel memory.

## Example
```nasm
; nasm -f elf32 fork.asm && i686-linux-gnu-ld -o fork fork.o && qemu-i386-static ./fork
section .data
    pmsg db "parent", 10
    cmsg db "child", 10

section .text
global _start
_start:
    mov eax, 2          ; fork()
    int 0x80
    test eax, eax
    jz .child           ; eax == 0 -> we are the child

    ; parent: write(1, pmsg, 7)
    mov eax, 4
    mov ebx, 1
    mov ecx, pmsg
    mov edx, 7
    int 0x80
    jmp .done
.child:
    mov eax, 4          ; write(1, cmsg, 6)
    mov ebx, 1
    mov ecx, cmsg
    mov edx, 6
    int 0x80
.done:
    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
Fork embodies UNIX's key insight: creating a process and loading a program are **separate** operations (fork/exec separation). One call, two return values, no configuration flags — the child inherits everything, and the code in between fork and exec (redirecting fds, dropping privileges) is just ordinary code. This is how every shell launches every command.

## In The Linux Programming Interface
Ch 24 "Process Creation" (p.513). The book details exactly what is and isn't inherited by the child, copy-on-write semantics, and the race conditions around which process runs first after fork.
