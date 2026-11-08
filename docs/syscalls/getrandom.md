[← Back to index](README.md)

# getrandom (syscall 355)

Fill a buffer with cryptographically secure random bytes.

## What it does
`getrandom` (added in Linux 3.17, 2014) gets random bytes straight from the kernel's random-number generator — no file descriptor needed. It fixes the annoyances of the classic approach (opening `/dev/urandom`): it works even when no fds are available or in a chroot with no `/dev`, and with `flags` = 0 it blocks until the kernel's entropy pool is initialized at early boot, so you never get weak randomness.

The `flags` argument (OR the bits together) changes that behaviour:

| Constant | Value | Meaning | Example |
|---|---|---|---|
| (none) | 0 | Read from the urandom source; block only until the pool is initialized at boot | `getrandom(buf, 16, 0)` — the normal choice |
| `GRND_NONBLOCK` | 1 | Fail with `-EAGAIN` (-11) instead of blocking when no randomness is ready yet | `getrandom(buf, 16, GRND_NONBLOCK)` in early-boot code that must not stall |
| `GRND_RANDOM` | 2 | Read from the `/dev/random` source; before Linux 5.6 this could block and return fewer bytes, and since 5.6 it behaves like 0 | rarely needed |
| `GRND_INSECURE` | 4 | Never block, even before initialization, accepting possibly weak bytes (Linux 5.6+) | non-cryptographic uses such as hash-table seeds |

## How it works
```c
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 355 |
| `ebx` | `buf` | **Out.** Address of your memory; the kernel writes the random bytes here. Must have room for `buflen` bytes |
| `ecx` | `buflen` | **In.** How many random bytes you want, in bytes. 0 returns 0 |
| `edx` | `flags` | **In.** Whether to block and which source to use; 0 is the normal choice. OR the bits from the table above |

Returns the number of random bytes written into `buf`, or `-errno`. Requests of up to 256 bytes always get the full amount; larger requests can come back short if a signal arrives, so check the count. Common errors: `-EINVAL` (-22, bad flags), `-EAGAIN` (-11, `GRND_NONBLOCK` set and no randomness ready yet), `-ENOSYS` (-38, kernel older than 3.17).

## Example
```nasm
; nasm -f elf32 rnd.asm && i686-linux-gnu-ld -o rnd rnd.o && qemu-i386-static ./rnd
; Writes 16 random bytes to stdout (pipe through xxd to view).
section .bss
    buf resb 16

section .text
global _start
_start:
    ; getrandom(buf, 16, 0)
    mov eax, 355
    mov ebx, buf
    mov ecx, 16
    xor edx, edx        ; flags = 0
    int 0x80

    ; write(1, buf, 16)
    mov eax, 4
    mov ebx, 1
    mov ecx, buf
    mov edx, 16
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

The classic pre-3.17 way, still everywhere in existing code:
```c
int fd = open("/dev/urandom", O_RDONLY);
read(fd, buf, 16);
close(fd);
```

## Linux philosophy
Randomness was originally exposed the pure UNIX way — as files, `/dev/random` and `/dev/urandom`, readable with plain `read`. `getrandom` is a rare, deliberate exception to "everything is a file", added because early-boot and no-fd situations made the file interface genuinely unsafe. Even Linux breaks its own rules when correctness demands it.

## In The Linux Programming Interface
No chapter — `getrandom` (2014) postdates TLPI (2010). The book covers the older approach instead: reading `/dev/urandom` via `open`+`read`, using the universal file I/O calls from Ch 4.
