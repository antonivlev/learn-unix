[← Back to index](README.md)

# mkdir (syscall 39) / rmdir (syscall 40)

Create and remove directories.

## What it does
`mkdir` creates a new, empty directory with the given permissions (the mode is filtered through the process umask, so `0o777` typically ends up as `0o755`). `rmdir` removes a directory — but only if it's already empty; there is deliberately no "recursive delete" syscall (`rm -r` does it by walking the tree itself).

The `mode` is three octal digits — owner, group, others, left to right — and each digit is a sum of the bits below. The actual permissions are `mode & ~umask`; with the common umask `0o022`, `0o777` becomes `0o755`.

| Bit | Value in a digit | Owner constant | Meaning for a directory |
|---|---|---|---|
| read | 4 | `S_IRUSR` = `0o400` | list the names inside (`ls`) |
| write | 2 | `S_IWUSR` = `0o200` | create, delete, and rename entries inside (also needs execute) |
| execute | 1 | `S_IXUSR` = `0o100` | "search": `cd` into it and reach entries by name |

Examples: `0o755` (`rwxr-xr-x`) lets everyone enter and list while only the owner changes contents; `0o700` (`rwx------`) makes a private directory.

## How it works
```c
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);
```

| Register | mkdir (39) | rmdir (40) |
|---|---|---|
| `eax` | 39 | 40 |
| `ebx` | pathname (NUL-terminated) | pathname (NUL-terminated) |
| `ecx` | mode (e.g. `0o755`) | — |

| Parameter | Meaning |
|---|---|
| `pathname` (mkdir) | **In.** Address of a NUL-terminated path string for the new directory, e.g. `db "mydir", 0`. Relative paths start from the current directory. The parent must already exist (there is no `mkdir -p` in the kernel) and the last component must not exist yet |
| `mode` (mkdir) | **In.** Permission bits for the new directory, written in octal; the umask is cleared from them. See the table above |
| `pathname` (rmdir) | **In.** Address of a NUL-terminated path string naming the directory to remove. It must be empty (only `.` and `..`); a final component of `.` fails with `-EINVAL` (-22) |

Both return 0 on success, meaning the directory now exists (mkdir) or is gone (rmdir); `-errno` on error. Common errors: `-EEXIST` (-17, mkdir: already exists), `-ENOTEMPTY` (-39, rmdir: directory not empty), `-ENOENT` (-2).

## Example
```nasm
; nasm -f elf32 dirs.asm && i686-linux-gnu-ld -o dirs dirs.o && qemu-i386-static ./dirs
section .data
    dname db "mydir", 0

section .text
global _start
_start:
    ; mkdir("mydir", 0755)
    mov eax, 39
    mov ebx, dname
    mov ecx, 0o755      ; rwxr-xr-x
    int 0x80

    ; rmdir("mydir")  -- remove it again
    mov eax, 40
    mov ebx, dname
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
#include <sys/stat.h>
#include <unistd.h>
mkdir("mydir", 0755);
rmdir("mydir");
```

## Linux philosophy
Directories are the joints of the hierarchical namespace, yet the kernel keeps their API minimal: create empty, remove empty, nothing more. Anything fancier (recursive create/delete) is composed in userspace from these primitives — small kernel surface, big userspace flexibility.

## In The Linux Programming Interface
Ch 18 "Directories and Links", §18.6. The book covers how mode interacts with the umask and provides a recursive tree-walking example (`nftw`) that shows how `rm -r`-style tools are built on top.
