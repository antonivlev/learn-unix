[← Back to index](README.md)

# lseek (syscall 19)

Move the read/write position ("file offset") of an open file.

## What it does

Every open file has a current offset: `read` and `write` operate there and advance it. `lseek` moves the offset explicitly. The `whence` argument says what `offset` is measured from:

| `whence` | Value | New offset is | Example |
|---|---|---|---|
| `SEEK_SET` | 0 | `offset` (absolute position from the start of the file; must be ≥ 0) | `lseek(fd, 0, SEEK_SET)` rewinds to the start |
| `SEEK_CUR` | 1 | current offset + `offset` (relative to where you are; may be negative) | `lseek(fd, 0, SEEK_CUR)` returns the current position without moving |
| `SEEK_END` | 2 | file size + `offset` (relative to the end; may be negative) | `lseek(fd, -10, SEEK_END)` goes to 10 bytes before the end |

Seeking past the end and then writing creates a "hole" (a sparse file). Seeking to a negative position fails with `-EINVAL`. Pipes, sockets, and terminals aren't seekable (`-ESPIPE`).

## How it works

```c
off_t lseek(int fd, off_t offset, int whence);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 19 |
| `ebx` | `fd` | **In.** File descriptor whose offset to move, usually a number returned by `open`. Must refer to something seekable (a regular file or block device) |
| `ecx` | `offset` | **In.** Distance in bytes, as a signed 32-bit number (−2³¹ to 2³¹−1). What it is measured from depends on `whence`; negative values move backwards |
| `edx` | `whence` | **In.** Selects the starting point for `offset`: 0, 1 or 2, see the table above. Any other value gives `-EINVAL` |

Return in `eax`: the new offset, in bytes from the start of the file (this is where the next `read` or `write` will happen), or negative `-errno`.

Common errors: `-EBADF` (-9), `-EINVAL` (-22, bad `whence` or the result would be negative), `-ESPIPE` (-29, fd is a pipe/socket — not seekable), `-EOVERFLOW` (-75, the new offset doesn't fit in 32 bits).

For files larger than 2 GB, the 32-bit offset isn't enough: use **_llseek (140)** — `_llseek(fd, offset_high, offset_low, result, whence)` in `ebx`..`edi` — which takes a 64-bit offset in two halves and stores the 64-bit result through a pointer. This is what libc's `lseek64` calls on 32-bit systems. Its parameters:

- `fd` (`ebx`) — **In.** Same as for `lseek`.
- `offset_high` (`ecx`) — **In.** Upper 32 bits of the signed 64-bit byte offset (0 for small positive offsets, 0xFFFFFFFF for small negative ones).
- `offset_low` (`edx`) — **In.** Lower 32 bits of the offset.
- `result` (`esi`) — **Out.** Address of 8 bytes of your memory (a `loff_t`); the kernel stores the new offset there, low dword at byte 0 and high dword at byte 4.
- `whence` (`edi`) — **In.** Same values as for `lseek`, see the table above.

`_llseek` returns 0 in `eax` on success (the offset itself is in `*result`), or negative `-errno`.

## Example

```nasm
; nasm -f elf32 seek.asm && i686-linux-gnu-ld -o seek seek.o && qemu-i386-static ./seek
; Gets a file's size, then exits with it: run  ./seek; echo $?
section .data
    fname db "/etc/hostname", 0

section .text
global _start
_start:
    mov eax, 5          ; open(fname, O_RDONLY)
    mov ebx, fname
    xor ecx, ecx
    int 0x80
    mov ebx, eax        ; fd

    ; size = lseek(fd, 0, SEEK_END)
    mov eax, 19
    xor ecx, ecx        ; offset 0
    mov edx, 2          ; SEEK_END
    int 0x80            ; eax = file size (new offset)

    mov ebx, eax        ; exit(size & 0xff)
    mov eax, 1
    int 0x80
```

```c
off_t size = lseek(fd, 0, SEEK_END);   /* classic "how big is this file?" */
```

## Linux philosophy

The file offset lives in the kernel's open-file description, not in your program — so the byte-stream abstraction stays stateless from the program's view (`read`/`write` need no position argument), and the offset is *shared* by fds duplicated with `dup` or inherited across `fork`. `lseek` is pure mechanism: random access built from one tiny call on top of sequential I/O.

## In The Linux Programming Interface

Ch 4 §4.7. The book adds file holes and sparse files, and clarifies which file types are seekable.
