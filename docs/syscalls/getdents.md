[← Back to index](README.md)

# getdents (syscall 141) / getdents64 (syscall 220)

Read directory entries — the syscall behind `ls` and `readdir(3)`.

## What it does
A directory is opened like a file (`open` with `O_RDONLY`), but you can't `read` it — you call `getdents` on the fd instead. The kernel fills your buffer with a batch of variable-length records, one per entry (including `.` and `..`). You call it repeatedly until it returns 0. Use `getdents64` (220) in new code; libc's `readdir(3)` is a friendly wrapper around exactly this.

## How it works
```c
long getdents64(int fd, void *dirp, unsigned int count);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 141 (getdents) or 220 (getdents64) |
| `ebx` | `fd` | **In.** File descriptor of a directory, returned by `open(path, O_RDONLY)`. The fd's position advances with each call, so the next call continues where the last one stopped |
| `ecx` | `dirp` | **Out.** Address of your memory; the kernel fills it with as many whole records as fit, laid out as shown below. Must have room for `count` bytes |
| `edx` | `count` | **In.** Size of `dirp` in bytes. 4096 is a common choice; if not even one record fits, the call fails with `-EINVAL` |

Returns the number of bytes of records written into `dirp` (walk only that many bytes; 0 = end of directory, no more entries), or `-errno`. Common errors: `-ENOTDIR` (-20, fd isn't a directory), `-EBADF` (-9), `-EINVAL` (-22, buffer size too small for even one record).

The syscall number in `eax` selects the record layout: 220 (`getdents64`) writes the `linux_dirent64` records below; 141 (`getdents`) writes the older `linux_dirent` layout, with 4-byte `d_ino`/`d_off`, `d_reclen` at offset 8, `d_name` at offset 10, and `d_type` stored in the record's last byte.

Each `linux_dirent64` record in the buffer:

| Offset | Size | Field |
|---|---|---|
| 0 | 8 | `d_ino` — inode number |
| 8 | 8 | `d_off` — position value for the kernel's use (don't interpret it) |
| 16 | 2 | `d_reclen` — total record length (hop by this) |
| 18 | 1 | `d_type` — file type (regular=8, dir=4, symlink=10) |
| 19 | var | `d_name` — NUL-terminated filename |

## Example
```nasm
; nasm -f elf32 ls.asm && i686-linux-gnu-ld -o ls ls.o && qemu-i386-static ./ls
; Minimal skeleton: open ".", read one batch of entries.
section .data
    dot db ".", 0
section .bss
    buf resb 4096

section .text
global _start
_start:
    ; fd = open(".", O_RDONLY)
    mov eax, 5
    mov ebx, dot
    xor ecx, ecx        ; O_RDONLY = 0
    int 0x80

    ; nread = getdents64(fd, buf, 4096)
    mov ebx, eax        ; fd
    mov eax, 220
    mov ecx, buf
    mov edx, 4096
    int 0x80
    ; now walk buf: each record's name is at +19, next record at +d_reclen (u16 at +16)

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
Directories really are files: you `open` and `close` them with the same calls as everything else, and `getdents` is essentially a structured `read`. The kernel provides only the raw record stream; sorting, formatting, and columns — everything `ls` adds — is userspace composition on top of one small primitive.

## In The Linux Programming Interface
Ch 18 "Directories and Links", §18.8 on reading directories. The book covers the portable `opendir`/`readdir`/`closedir` API you'd use in real C code, which is built on getdents.
