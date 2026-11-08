[← Back to index](README.md)

# stat / fstat (syscalls 106 / 108) and stat64 / fstat64 (195 / 197)

Get a file's metadata: size, permissions, owner, timestamps, type.

## What it does

`stat` fills a `struct stat` with everything the filesystem knows *about* a file — without opening it: size, mode (type + permissions), owner, inode number, and access/modify/change times. `fstat` does the same for an already-open fd. This is how `ls -l` gets its columns.

## How it works

```c
int stat(const char *pathname, struct stat *buf);
int fstat(int fd, struct stat *buf);
```

| Register | stat (106) | fstat (108) | stat64 (195) | fstat64 (197) |
|---|---|---|---|---|
| `eax` | 106 | 108 | 195 | 197 |
| `ebx` | pathname | fd | pathname | fd |
| `ecx` | stat buf | stat buf | stat64 buf | stat64 buf |

| Parameter | Meaning |
|---|---|
| `pathname` | **In.** Address of a NUL-terminated path string, e.g. `db "/etc/hostname", 0`. Relative paths start from the current directory. Symlinks are followed, so you get the metadata of the file the link points to |
| `fd` | **In.** File descriptor of an already-open file: a number returned by `open`, `pipe`, `socket`, etc., or 0/1/2 for stdin/stdout/stderr |
| `buf` | **Out.** Address of your memory; the kernel writes the whole struct here. Needs room for 64 bytes (`struct stat`, syscalls 106/108) or 96 bytes (`struct stat64`, syscalls 195/197/196). The fields are listed below |

`lstat64` (196) takes the same `pathname` and `buf` as `stat64`; the difference is that when `pathname` is a symlink, it describes the link itself and leaves the link unfollowed.

`struct stat64` as the kernel writes it on i386 (96 bytes; from `arch/x86/include/uapi/asm/stat.h`):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 8 | `st_dev` | ID of the device (filesystem) holding the file |
| 12 | 4 | `__st_ino` | low 32 bits of the inode number (historical; use `st_ino` at 88) |
| 16 | 4 | `st_mode` | file type + permissions. Type is `st_mode & 0o170000`: `0o100000` regular file, `0o040000` directory, `0o120000` symlink. The low 12 bits are the permissions, e.g. `0o644` |
| 20 | 4 | `st_nlink` | number of hard links (names) pointing at the file |
| 24 | 4 | `st_uid` | user ID of the owner |
| 28 | 4 | `st_gid` | group ID of the owner |
| 32 | 8 | `st_rdev` | device number, only meaningful for device files (`/dev/...`) |
| 44 | 8 | `st_size` | size in bytes (for a symlink: length of the target path) |
| 52 | 4 | `st_blksize` | preferred I/O block size in bytes |
| 56 | 8 | `st_blocks` | disk space allocated, in 512-byte units |
| 64 | 4 | `st_atime` | last access time, seconds since 1970-01-01 UTC (nanoseconds part at 68) |
| 72 | 4 | `st_mtime` | last modification of the contents, seconds (nanoseconds at 76) |
| 80 | 4 | `st_ctime` | last status change (permissions, owner, links), seconds (nanoseconds at 84) |
| 88 | 8 | `st_ino` | inode number |

Offsets 8–11 and 40–43 are padding. The old 64-byte `struct stat` (106/108) packs fields tighter: `st_dev` (4 bytes) at 0, `st_ino` (4) at 4, `st_mode` (2) at 8, `st_nlink` (2) at 10, `st_uid` (2) at 12, `st_gid` (2) at 14, `st_size` (4) at 20, `st_mtime` (4) at 40.

Return in `eax`: 0 on success, meaning the struct at `buf` has been filled in; negative `-errno` on error.

Common errors: `-ENOENT` (-2, no such file), `-EBADF` (-9, for fstat).

**Why the 64 variants?** The old 32-bit `struct stat` uses 32-bit fields for file size and inode number, so it simply can't describe a file over 2 GB (`stat` fails with `-EOVERFLOW`). `stat64`/`fstat64` use a larger struct with 64-bit fields. On 32-bit systems, libc's `stat()` actually calls the 64 variants under the hood — use them in new code. (`lstat64` is 196; it doesn't follow symlinks.)

## Example

```nasm
; nasm -f elf32 stat.asm && i686-linux-gnu-ld -o stat stat.o && qemu-i386-static ./stat
; Exits 0 if the file exists, 1 if not: ./stat; echo $?
section .data
    fname db "/etc/hostname", 0

section .bss
    statbuf resb 144            ; struct stat64 is 96 bytes; 144 is safely big

section .text
global _start
_start:
    mov eax, 195        ; stat64
    mov ebx, fname      ; pathname
    mov ecx, statbuf    ; where the kernel writes the struct
    int 0x80

    test eax, eax
    js .missing
    xor ebx, ebx        ; exit(0): file exists
    jmp .exit
.missing:
    mov ebx, 1          ; exit(1)
.exit:
    mov eax, 1
    int 0x80
```

```c
struct stat sb;
if (stat("/etc/hostname", &sb) == 0)
    printf("%lld bytes\n", (long long)sb.st_size);
```

## Linux philosophy

The `stat` structure is the universal description of *any* filesystem object — regular file, directory, device node, socket, symlink all answer the same questions, distinguished only by bits in `st_mode`. The 64-bit variants show how Linux evolves ABIs: old syscalls are never broken, new numbers are added alongside, and libc quietly picks the right one.

## In The Linux Programming Interface

Ch 15 "File Attributes" (p.279), §15.1. The book walks through every `stat` field, the `S_ISREG()`-style type macros, and the exact meaning of the three timestamps.
