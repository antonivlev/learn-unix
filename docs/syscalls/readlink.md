[← Back to index](README.md)

# readlink (syscall 85)

Read the target string stored inside a symbolic link.

## What it does
A symbolic link is a small file whose content is just a pathname. Most syscalls (`open`, `stat`, ...) follow symlinks automatically; `readlink` is how you look at the link *itself* and get the path it points to. **The result is NOT NUL-terminated** — you must use the returned length (or add a NUL yourself before treating it as a C string).

## How it works
```c
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 85 |
| `ebx` | `pathname` | **In.** Address of a NUL-terminated path string naming the symlink itself, e.g. `db "/proc/self/exe", 0`. The link in the last component is read and left unfollowed |
| `ecx` | `buf` | **Out.** Address of your memory; the kernel copies the link's target path here with no trailing NUL. Must have room for `bufsiz` bytes |
| `edx` | `bufsiz` | **In.** Size of `buf` in bytes; the kernel writes at most this many and silently cuts off a longer target. 0 fails with `-EINVAL`. To add a NUL yourself, pass one less than the buffer size |

Returns the length of the target path in bytes, i.e. how many bytes were placed in `buf` (no trailing NUL; if it equals `bufsiz`, the result may be truncated), or `-errno`. Common errors: `-EINVAL` (-22, not a symlink), `-ENOENT` (-2).

## Example
```nasm
; nasm -f elf32 rl.asm && i686-linux-gnu-ld -o rl rl.o && qemu-i386-static ./rl
; Prints the target of /proc/self/exe (a symlink to this binary).
section .data
    lnk db "/proc/self/exe", 0
section .bss
    buf resb 256

section .text
global _start
_start:
    ; len = readlink("/proc/self/exe", buf, 256)
    mov eax, 85
    mov ebx, lnk
    mov ecx, buf
    mov edx, 256
    int 0x80

    test eax, eax
    js .exit

    ; write(1, buf, len)  -- len from readlink, NOT NUL-terminated
    mov edx, eax
    mov eax, 4
    mov ebx, 1
    mov ecx, buf
    int 0x80

.exit:
    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
Symlinks show "everything is a file" at work: a redirection in the namespace is itself just a tiny file holding a path, readable like any data. And `/proc` doubles down — the kernel exposes live process facts (like the running executable's path) as symlinks you inspect with this one ordinary syscall.

## In The Linux Programming Interface
Ch 18 "Directories and Links", §18.5 on symbolic links. The book contrasts hard vs. symbolic links, explains dangling links, and lists exactly which syscalls follow symlinks and which don't.
