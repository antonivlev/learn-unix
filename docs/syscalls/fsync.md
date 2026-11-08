[← Back to index](README.md)

# fsync (syscall 118)

Force a file's buffered data (and metadata) out to the disk.

## What it does

When `write` returns, your bytes are usually only in the kernel's buffer cache — the disk write happens later, whenever the kernel feels like it. `fsync` blocks until the file's data *and* metadata (size, timestamps) are physically on the storage device. It's what databases and editors call before claiming "your data is saved". It can be slow; that's the point.

## How it works

```c
int fsync(int fd);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 118 |
| `ebx` | `fd` | **In.** File descriptor of the file to flush, usually a number returned by `open` that you've been `write`ing to. Pipes, sockets and terminals have nothing to flush and give `-EINVAL` |

Return in `eax`: 0 on success, meaning everything written to the file so far is on the storage device; negative `-errno`.

Common errors: `-EBADF` (-9, bad fd), `-EIO` (-5, an actual I/O error surfaced while flushing), `-EINVAL` (-22, the fd refers to something that doesn't support syncing).

Related: `sync` (36) takes no arguments and flushes *all* filesystem buffers system-wide; `fdatasync` (148) takes the same single `fd` argument in `ebx` and flushes data but skips non-essential metadata (such as timestamps) for speed.

## Example

```nasm
; nasm -f elf32 fsync.asm && i686-linux-gnu-ld -o fsync fsync.o && qemu-i386-static ./fsync
section .data
    fname  db "durable.txt", 0
    msg    db "safely on disk", 10
    msglen equ $ - msg

section .text
global _start
_start:
    mov eax, 5          ; fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov ebx, fname
    mov ecx, 0x241
    mov edx, 0o644
    int 0x80
    mov ebx, eax        ; fd (stays in ebx for the next 3 calls)

    mov eax, 4          ; write(fd, msg, msglen)
    mov ecx, msg
    mov edx, msglen
    int 0x80

    mov eax, 118        ; fsync(fd) -- returns only when it's on disk
    int 0x80

    mov eax, 6          ; close(fd)
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy

Separating `write` (fast, buffered) from `fsync` (slow, durable) is mechanism-not-policy in its purest form: the kernel provides both behaviors and *you* choose when durability is worth the cost. Most programs never need it; the few that do (databases, mail servers) get an explicit, per-file knob instead of a system-wide slowdown.

## In The Linux Programming Interface

Ch 13 "File I/O Buffering" (p.233), §13.3. The book maps out the full buffering stack (stdio buffers, kernel buffer cache, disk cache) and compares `fsync`, `fdatasync`, `sync`, and the `O_SYNC` open flag.
