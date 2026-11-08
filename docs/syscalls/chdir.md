[← Back to index](README.md)

# chdir (syscall 12) / getcwd (syscall 183)

Change and query the current working directory.

## What it does
Every process carries a **current working directory (cwd)** as part of its own state — it's where all relative paths (like `"out.txt"`) start resolving from. `chdir` changes it; `getcwd` writes the absolute path of it into a buffer you supply. Because cwd is per-process, `cd` must be a shell *builtin*: a child process changing its own cwd would not affect the parent shell.

## How it works
```c
int chdir(const char *path);
char *getcwd(char *buf, size_t size);   /* libc wrapper */
```

| Register | chdir (12) | getcwd (183) |
|---|---|---|
| `eax` | 12 | 183 |
| `ebx` | path (NUL-terminated) | buffer |
| `ecx` | — | buffer size |

| Parameter | Meaning |
|---|---|
| `path` (chdir) | **In.** Address of a NUL-terminated path string naming the new working directory, e.g. `db "/tmp", 0`. A relative path starts from the current directory (so `".."` goes up one level). It must be a directory you have execute (search) permission on; symlinks are followed |
| `buf` (getcwd) | **Out.** Address of your memory; the kernel writes the absolute path of the cwd here, ending with a NUL. Must have room for `size` bytes. Passing NULL to get an allocated buffer is a glibc feature; the raw syscall fails with `-EFAULT` (-14) |
| `size` (getcwd) | **In.** Size of `buf` in bytes. It must cover the whole path plus its NUL, or the call fails with `-ERANGE` and writes nothing. 4096 (`PATH_MAX`) always fits |

`chdir` returns 0 on success, meaning relative paths now resolve from the new directory. The raw `getcwd` syscall returns the string length **including** the NUL on success. Both return `-errno` on error: `-ENOENT` (-2, path doesn't exist), `-ENOTDIR` (-20), `-ERANGE` (-34, getcwd buffer too small).

## Example
```nasm
; nasm -f elf32 cwd.asm && i686-linux-gnu-ld -o cwd cwd.o && qemu-i386-static ./cwd
section .data
    path db "/tmp", 0
section .bss
    buf  resb 256

section .text
global _start
_start:
    ; chdir("/tmp")
    mov eax, 12
    mov ebx, path
    int 0x80

    ; getcwd(buf, 256) -> eax = length incl. NUL
    mov eax, 183
    mov ebx, buf
    mov ecx, 256
    int 0x80

    ; write(1, buf, len-1)  -- print cwd without the NUL
    lea edx, [eax - 1]
    mov eax, 4
    mov ebx, 1
    mov ecx, buf
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
The current directory is per-process state that makes the huge shared filesystem convenient: relative paths let programs run anywhere in the tree. A child inherits it at `fork`; later changes affect only itself.

## In The Linux Programming Interface
Ch 18 "Directories and Links", §18.10. The book also covers `fchdir` (change cwd via an open fd) and how getcwd is implemented, plus the related notion of a per-process root directory (`chroot`).
