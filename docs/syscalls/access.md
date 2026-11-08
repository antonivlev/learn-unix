[← Back to index](README.md)

# access (syscall 33)

Check whether a file exists or is readable/writable/executable.

## What it does
`access` answers "could I open this?" without actually opening it. The `mode` argument says which question to ask:

| `mode` | Value | Succeeds if | Example |
|---|---|---|---|
| `F_OK` | 0 | the file exists (no permission check) | `access("out.txt", F_OK)` |
| `X_OK` | 1 | you may execute it (for a directory: search it, i.e. `cd` into it) | `access("/bin/ls", X_OK)` |
| `W_OK` | 2 | you may write it | `access("out.txt", W_OK)` |
| `R_OK` | 4 | you may read it | `access("/etc/passwd", R_OK)` |

OR the bits together to ask several at once: `R_OK|W_OK` = 6 returns 0 only if both are granted. It checks against the *real* (not effective) user ID, which matters mainly in set-UID programs (see [glossary](../glossary.md)).

## How it works
```c
int access(const char *pathname, int mode);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 33 |
| `ebx` | `pathname` | **In.** Address of a NUL-terminated path string, e.g. `db "/etc/passwd", 0`. Relative paths start from the current directory; symlinks are followed, so the target is checked |
| `ecx` | `mode` | **In.** Which checks to make: `F_OK` (0) for existence only, or any OR of `R_OK` (4), `W_OK` (2), `X_OK` (1); see the table above |

Returns 0 if the file exists and every requested permission is granted, `-errno` otherwise. Common errors: `-ENOENT` (-2, doesn't exist), `-EACCES` (-13, permission denied).

**Caveat — check-then-use race** (see [glossary](../glossary.md): race condition): between your `access` check and your later `open`, the file can be replaced or its permissions changed. Never use `access` as a security check; just `open` and handle the error.

## Example
```nasm
; nasm -f elf32 acc.asm && i686-linux-gnu-ld -o acc acc.o && qemu-i386-static ./acc
; Exit 0 if /etc/passwd is readable, 1 otherwise.
section .data
    fname db "/etc/passwd", 0

section .text
global _start
_start:
    ; access("/etc/passwd", R_OK)
    mov eax, 33
    mov ebx, fname
    mov ecx, 4          ; R_OK
    int 0x80

    mov ebx, 0
    test eax, eax
    jz .done
    mov ebx, 1
.done:
    mov eax, 1          ; exit
    int 0x80
```

```c
#include <unistd.h>
if (access("/etc/passwd", R_OK) == 0) { /* probably readable */ }
```

## Linux philosophy
`access` is a lesson in UNIX's preference for **asking forgiveness over permission**: because the filesystem is shared, mutable state, any check-then-act sequence is racy. The idiomatic design is to attempt the operation (`open`) and let the error code tell you why it failed — errors are a normal, composable part of the API, not an exception.

## In The Linux Programming Interface
Ch 15 "File Attributes" (p.279), §15.4.3. The book explains the real-vs-effective UID distinction that motivated `access` and spells out the check-then-use race (the book calls it TOCTOU) with a concrete attack.
