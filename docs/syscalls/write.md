[← Back to index](README.md)

# write (syscall 4)

Write bytes from a buffer to a file descriptor.

## What it does

`write` sends up to `count` bytes from your buffer to whatever the fd refers to — a file, the terminal, a pipe, a socket. It can write fewer bytes than requested (e.g. the disk fills up, a signal interrupts it, or a non-blocking pipe has room for only part of the data), so careful code loops until everything is written.

`fd` 1 is standard output and 2 is standard error by convention. Data goes at the fd's current offset, which then advances; if the fd was opened with `O_APPEND`, every `write` goes to the end of the file instead. When a pipe or socket is full, `write` waits for room; with `O_NONBLOCK` set it returns `-EAGAIN` (-11) instead. Note: a successful `write` means the *kernel* accepted the data, not that it's on disk yet (see `fsync`).

## How it works

```c
ssize_t write(int fd, const void *buf, size_t count);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 4 |
| `ebx` | `fd` | **In.** File descriptor to write to: a number returned by `open`, `pipe`, `socket`, etc., or 1 for stdout, 2 for stderr. Must be open for writing |
| `ecx` | `buf` | **In.** Address of the bytes to send; the kernel only reads this memory. No NUL terminator is needed, since the length comes from `count` |
| `edx` | `count` | **In.** Number of bytes to write from `buf`. 0 writes nothing and returns 0 |

Return in `eax`: number of bytes actually written (the first `eax` bytes of `buf`; if it is less than `count`, call `write` again for the rest); negative means `-errno`.

Common errors: `-EBADF` (-9, fd not open for writing), `-EPIPE` (-32, writing to a pipe with no reader; the kernel first sends `SIGPIPE`, which kills the process by default, so you only see `-EPIPE` if `SIGPIPE` is ignored or caught).

## Example

```nasm
; nasm -f elf32 write.asm && i686-linux-gnu-ld -o write write.o && qemu-i386-static ./write
section .data
    msg    db "hello from write", 10   ; 10 = '\n'
    msglen equ $ - msg

section .text
global _start
_start:
    mov eax, 4          ; write
    mov ebx, 1          ; fd 1 = stdout
    mov ecx, msg        ; buffer
    mov edx, msglen     ; count
    int 0x80            ; eax = bytes written

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
write(1, "hi\n", 3);   /* fd 1 = stdout, fd 2 = stderr */
```

## Linux philosophy

Like `read`, `write` treats every destination as an untyped byte stream behind a file descriptor — the same three arguments print to a terminal, append to a log, or send over a network. The kernel buffers writes and decides *when* to hit the disk (mechanism), leaving durability decisions to the program via `fsync` (policy stays with you).

## In The Linux Programming Interface

Ch 4 (p.69), §4.5 covers the call itself; Ch 13 "File I/O Buffering" (p.233) explains what really happens to your bytes — the kernel buffer cache, stdio buffering on top, and how to control both.
