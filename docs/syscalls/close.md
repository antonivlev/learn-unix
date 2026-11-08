[← Back to index](README.md)

# close (syscall 6)

Release a file descriptor.

## What it does

`close` tells the kernel you're done with an fd. The number becomes free for reuse by the next `open`/`dup`/`pipe`, and when the last fd referring to an open file is closed, the kernel releases the underlying resources. Processes have an fd limit (see `ulimit -n`), so leaking fds in long-running programs eventually makes `open` fail.

## How it works

```c
int close(int fd);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 6 |
| `ebx` | `fd` | **In.** File descriptor to release: a number returned by `open`, `dup`, `pipe`, `socket`, etc. After the call the number is free, so don't use it again |

Return in `eax`: 0 on success (the fd is closed), negative `-errno` on failure.

Common errors: `-EBADF` (-9, fd wasn't open). Rarely, `close` can also report deferred write errors (e.g. `-EIO` on NFS) — one reason careful programs check its return value.

## Example

```nasm
; nasm -f elf32 close.asm && i686-linux-gnu-ld -o close close.o && qemu-i386-static ./close
section .data
    fname db "/etc/hostname", 0

section .text
global _start
_start:
    ; fd = open("/etc/hostname", O_RDONLY)
    mov eax, 5
    mov ebx, fname
    xor ecx, ecx        ; O_RDONLY = 0
    int 0x80
    test eax, eax
    js .done            ; open failed, nothing to close

    ; close(fd)
    mov ebx, eax        ; fd from open
    mov eax, 6
    int 0x80            ; eax = 0 on success

.done:
    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
int fd = open("f", O_RDONLY);
if (fd >= 0) close(fd);
```

## Linux philosophy

`close` completes the minimal open/read/write/close lifecycle — four small primitives that cover all I/O. Because fds are just per-process integers in a table, closing one is cheap and local; the kernel handles the shared bookkeeping (the file stays open for other processes or duplicated fds until the last reference goes away).

## In The Linux Programming Interface

Ch 4 §4.6. The book explains why fd numbers get reused, what happens at process exit (everything is closed automatically), and why checking `close`'s return value matters more than it looks.
