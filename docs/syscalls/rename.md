[← Back to index](README.md)

# rename (syscall 38)

Rename or move a file or directory.

## What it does
`rename` changes a file's name, or moves it to a different directory. If the destination already exists, it is atomically replaced — there is never a moment where the destination is missing. It only manipulates directory entries, so no file data is copied — which also means it fails across filesystems (`mv` falls back to copy+delete in that case).

## How it works
```c
int rename(const char *oldpath, const char *newpath);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 38 |
| `ebx` | `oldpath` | **In.** Address of a NUL-terminated string: the current name of an existing file or directory. Relative paths start from the current directory. A symlink is renamed itself; its target is untouched |
| `ecx` | `newpath` | **In.** Address of a NUL-terminated string: the name it should have afterwards, in the same or another directory on the same filesystem. If `newpath` already exists it is replaced; when `oldpath` is a directory, an existing `newpath` must be an empty directory |

Returns 0 on success, meaning the file is now reachable only under `newpath`; `-errno` on error. Common errors: `-ENOENT` (-2, oldpath doesn't exist), `-EXDEV` (-18, old and new are on different filesystems).

## Example
```nasm
; nasm -f elf32 mv.asm && i686-linux-gnu-ld -o mv mv.o && qemu-i386-static ./mv
section .data
    oldname db "draft.txt", 0
    newname db "final.txt", 0

section .text
global _start
_start:
    ; rename("draft.txt", "final.txt")
    mov eax, 38
    mov ebx, oldname
    mov ecx, newname
    int 0x80

    mov ebx, 0
    test eax, eax
    jns .done
    mov ebx, 1
.done:
    mov eax, 1          ; exit
    int 0x80
```

```c
#include <stdio.h>
rename("draft.txt", "final.txt");
```

## Linux philosophy
Within one filesystem, `rename` is **atomic**: other processes see either the old file or the new one, never a half-state. This single guarantee is the foundation of safe file updates everywhere on UNIX — write to a temp file, then `rename` it over the real one. A small primitive with a strong contract beats a complicated "safe save" API.

## In The Linux Programming Interface
Ch 18 "Directories and Links", §18.4. The book walks through the exact rules (what happens when newpath exists, is a directory, is a hard link to oldpath) and the atomicity guarantee.
