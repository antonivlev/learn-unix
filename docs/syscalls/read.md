[← Back to index](README.md)

# read (syscall 3)

Read bytes from a file descriptor into a buffer.

## What it does

`read` pulls up to `count` bytes from whatever the file descriptor refers to — a file, a pipe, a terminal, a socket — into your buffer. It's one half of the universal I/O model: once something is open, you `read` from it the same way regardless of what it is. It may return fewer bytes than you asked for, and that's normal (not an error): a terminal returns one line at a time, a pipe or socket returns whatever has arrived, and a file returns only what's left before the end.

`fd` 0 is standard input by convention (1 is stdout, 2 is stderr). When no data is available yet, `read` waits; if the fd has `O_NONBLOCK` set it returns `-EAGAIN` (-11) immediately instead. A `count` of 0 returns 0 without reading anything.

## How it works

```c
ssize_t read(int fd, void *buf, size_t count);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 3 |
| `ebx` | `fd` | **In.** File descriptor to read from: a number returned by `open`, `pipe`, `socket`, etc., or 0 for stdin |
| `ecx` | `buf` | **Out.** Address of your memory; the kernel copies the bytes it reads here. Must have room for `count` bytes |
| `edx` | `count` | **In.** Maximum number of bytes to read, usually the size of `buf`. The kernel may read fewer |

Return in `eax`: number of bytes actually read (they are the first `eax` bytes of `buf`, and the file offset has moved forward by that much); `0` means end of file; negative means `-errno`.

Common errors: `-EBADF` (-9, bad fd), `-EFAULT` (-14, buffer pointer is invalid).

## Example

```nasm
; nasm -f elf32 read.asm && i686-linux-gnu-ld -o read read.o && qemu-i386-static ./read
section .bss
    buf resb 64

section .text
global _start
_start:
    mov eax, 3          ; read
    mov ebx, 0          ; fd 0 = stdin
    mov ecx, buf        ; buffer
    mov edx, 64         ; count
    int 0x80            ; eax = bytes read (0 = EOF)

    mov edx, eax        ; echo it back: count = bytes read
    mov eax, 4          ; write
    mov ebx, 1          ; stdout
    int 0x80            ; ecx still points at buf

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
char buf[64];
ssize_t n = read(0, buf, sizeof(buf));  /* n bytes, 0 = EOF, -1 = error */
```

## Linux philosophy

`read` is the same call whether the fd is a disk file, keyboard input, a pipe from another process, or a network socket — the fd is a universal handle and "everything is a file". Because it deals only in flat byte streams, it imposes no structure (no records, no types): mechanism, not policy. Programs written against `read` compose freely in pipelines.

## In The Linux Programming Interface

Ch 4 "File I/O: The Universal I/O Model" (p.69), §4.4. The book explains partial reads, why EOF is `0` rather than an error, and how the same model covers all file types.
