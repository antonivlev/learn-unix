[← Back to index](README.md)

# writev (syscall 146)

Write from several buffers in a single syscall ("gather" I/O).

## What it does

`writev` takes an array of `struct iovec` (pointer + length pairs) and writes them all out, in order, as one atomic operation. Useful when your data is naturally in pieces — say, a header in one buffer and a body in another — and you want one syscall instead of several `write`s (or an ugly copy into a temporary buffer). Its twin **readv (145)** does the same for reading ("scatter" I/O), filling each buffer in turn.

## How it works

```c
struct iovec { void *iov_base; size_t iov_len; };   /* 8 bytes on 32-bit */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 146 (writev) / 145 (readv) |
| `ebx` | `fd` | **In.** File descriptor to write to (or read from, for `readv`): a number returned by `open`, `pipe`, `socket`, etc., or 1 for stdout |
| `ecx` | `iov` | **In.** Address of an array of `iovcnt` `struct iovec` entries, 8 bytes each, laid out back to back (see below). The kernel only reads the array itself |
| `edx` | `iovcnt` | **In.** Number of entries in the `iov` array (elements, not bytes), from 0 to `IOV_MAX` = 1024 |

Each `struct iovec` entry describes one buffer:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `iov_base` | Address of the buffer. For `writev` the kernel reads bytes from it; for `readv` it writes bytes into it (**Out.**) |
| 4 | 4 | `iov_len` | Length of that buffer in bytes. For `readv`, the buffer must have room for this many bytes |

The buffers are processed in array order: `writev` sends all of buffer 0, then buffer 1, and so on; `readv` fills buffer 0 completely before moving on to buffer 1. An `iovcnt` of 0 writes nothing and returns 0; entries with `iov_len` = 0 are skipped.

Return in `eax`: total bytes transferred across all buffers (the sum of the `iov_len`s on a full transfer; it can be less, just like `write`/`read`; for `readv`, 0 means end of file), or negative `-errno`.

Common errors: `-EBADF` (-9), `-EINVAL` (-22, iovcnt negative or above 1024, or the lengths add up to more than fits in `ssize_t`).

## Example

```nasm
; nasm -f elf32 writev.asm && i686-linux-gnu-ld -o writev writev.o && qemu-i386-static ./writev
section .data
    part1  db "Hello, "
    len1   equ $ - part1
    part2  db "gathered world!", 10
    len2   equ $ - part2

    ; struct iovec[2]: each entry is { dd base, dd len }
    iov    dd part1, len1
           dd part2, len2

section .text
global _start
_start:
    mov eax, 146        ; writev
    mov ebx, 1          ; stdout
    mov ecx, iov        ; iovec array
    mov edx, 2          ; 2 iovecs
    int 0x80            ; eax = total bytes written

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
struct iovec iov[2] = { { "Hello, ", 7 }, { "world\n", 6 } };
writev(1, iov, 2);
```

## Linux philosophy

`writev` extends `write` without changing the model — same fd, same byte stream, just multiple sources gathered atomically. Atomicity matters: two separate `write`s from different processes can interleave in a pipe or log file, but one `writev` cannot (for a pipe, as long as the total is at most `PIPE_BUF` = 4096 bytes). It's a small, targeted addition rather than a new I/O framework.

## In The Linux Programming Interface

Ch 5 §5.7 "Scatter-Gather I/O" (p.89 for the chapter). The book covers `readv` in detail, the atomicity guarantee, and when scatter-gather beats a copy into one buffer.
