[← Back to index](README.md)

# unlink (syscall 10)

Delete a file by removing its directory entry.

## What it does
`unlink` removes a name from the filesystem. Every file has a **link count** (how many names point to it — see `ln`). `unlink` decrements that count; the file's data is only actually deleted when the count hits 0 **and** no process still holds it open via a file descriptor. That's why a program can `open` a temp file, `unlink` it immediately, and keep using it — the data vanishes automatically on `close`.

## How it works
```c
int unlink(const char *pathname);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 10 |
| `ebx` | `pathname` | **In.** Address of a NUL-terminated path string naming the file to remove, e.g. `db "out.txt", 0`. Relative paths start from the current directory. If it names a symlink, the link itself is removed and its target is untouched |

Returns 0 on success, meaning the name is gone (the data goes too once no other names or open fds refer to it); `-errno` on error. Common errors: `-ENOENT` (-2, no such file), `-EISDIR` (-21, it's a directory — use `rmdir` instead), `-EACCES` (-13, no write permission on the containing directory).

## Example
```nasm
; nasm -f elf32 rm.asm && i686-linux-gnu-ld -o rm rm.o && qemu-i386-static ./rm
section .data
    fname db "out.txt", 0

section .text
global _start
_start:
    ; unlink("out.txt")
    mov eax, 10
    mov ebx, fname
    int 0x80

    ; exit(0 if ok, 1 if error)
    mov ebx, 0
    test eax, eax
    jns .done
    mov ebx, 1
.done:
    mov eax, 1          ; exit
    int 0x80
```

```c
#include <unistd.h>
unlink("out.txt");   /* returns 0 or -1 (errno set) */
```

## Linux philosophy
The name is telling: you don't "delete a file", you *unlink a name from it*. Names and file data (inodes, see [glossary](../glossary.md)) are separate things — the directory tree is just a layer of names pointing at inodes. This separation gives you hard links, and the elegant open-then-unlink trick for self-cleaning temporary files.

## In The Linux Programming Interface
Ch 18 "Directories and Links" (p.339), §18.3. The book explains the link-count model in depth and shows the classic unlink-while-open temp-file idiom with example code.
