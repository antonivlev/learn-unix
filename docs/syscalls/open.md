[← Back to index](README.md)

# open (syscall 5)

Open (or create) a file and get back a file descriptor.

## What it does

`open` turns a pathname into a file descriptor — the small integer you then pass to `read`, `write`, `lseek`, and `close`. Flags say how you want the file opened (read/write, create it, truncate it, append), and `mode` gives the permissions if a new file is created. The kernel returns the lowest unused fd number.

`flags` is exactly one access mode OR'd with any number of the other flags:

| Flag | Value | Meaning | Example |
|---|---|---|---|
| `O_RDONLY` | 0 | Access mode: read only | `open("f", O_RDONLY)` |
| `O_WRONLY` | 1 | Access mode: write only | `open("log", O_WRONLY\|O_APPEND)` |
| `O_RDWR` | 2 | Access mode: read and write | `open("db", O_RDWR)` |
| `O_CREAT` | 0x40 | Create the file if it doesn't exist, with permissions from `mode` | `open("f", O_WRONLY\|O_CREAT, 0o644)` |
| `O_EXCL` | 0x80 | With `O_CREAT`: fail with `-EEXIST` if the file already exists (atomic "create new only", used for lock files) | `open("lock", O_WRONLY\|O_CREAT\|O_EXCL, 0o600)` |
| `O_TRUNC` | 0x200 | If the file exists and is opened for writing, cut its length to 0 | `open("out", O_WRONLY\|O_CREAT\|O_TRUNC, 0o644)` = flags 0x241 |
| `O_APPEND` | 0x400 | Every `write` first moves the offset to the end of the file (atomically) | `open("log", O_WRONLY\|O_CREAT\|O_APPEND, 0o644)` = flags 0x441 |
| `O_NONBLOCK` | 0x800 | `read`/`write` return `-EAGAIN` when they would otherwise wait (pipes, FIFOs, terminals, sockets) | `open("fifo", O_RDONLY\|O_NONBLOCK)` |
| `O_LARGEFILE` | 0x8000 | Allow files larger than 2 GB on 32-bit; without it such opens fail with `-EOVERFLOW`. libc adds it for you | `open("big.iso", O_RDONLY\|O_LARGEFILE)` |
| `O_DIRECTORY` | 0x10000 | Fail with `-ENOTDIR` unless the path is a directory | `open("/tmp", O_RDONLY\|O_DIRECTORY)` |
| `O_CLOEXEC` | 0x80000 | Set close-on-exec: the fd is closed automatically when the process calls `execve` | `open("f", O_RDONLY\|O_CLOEXEC)` |

`mode` matters only when `O_CREAT` actually creates the file; otherwise the kernel ignores it. It is the permission bits in octal: the three digits are owner, group, and others, and each digit adds 4 = read, 2 = write, 1 = execute (`0o644` = `rw-r--r--`, `0o755` = `rwxr-xr-x`). The process umask is subtracted first, so `0o666` with the usual umask `0o022` gives `0o644`.

## How it works

```c
int open(const char *pathname, int flags, mode_t mode);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 5 |
| `ebx` | `pathname` | **In.** Address of the file's path as a NUL-terminated string (end it with a `0` byte, e.g. `db "out.txt", 0`). A relative path is resolved from the current directory |
| `ecx` | `flags` | **In.** One access mode OR'd with optional flags; controls read/write access, creation, truncation and appending. See the table above |
| `edx` | `mode` | **In.** Permission bits for a newly created file, e.g. `0o644`; the umask is subtracted first. Ignored unless `O_CREAT` creates the file, so any value (usually 0) works otherwise |

Return in `eax`: the new file descriptor (>= 0), the lowest fd number not already open in this process; pass it to `read`, `write`, `lseek` and `close`. Negative means `-errno`.

Common errors: `-ENOENT` (-2, no such file and no `O_CREAT`), `-EACCES` (-13, permission denied), `-EEXIST` (-17, file exists and `O_CREAT|O_EXCL` was given).

Relatives:

- **creat (8)** — `creat(path, mode)` in `ebx`,`ecx`: shorthand for `open` with `O_CREAT|O_WRONLY|O_TRUNC`. `path` is the NUL-terminated pathname and `mode` the permission bits for a new file, both exactly as for `open`; there is no `flags` argument. Returns a write-only fd. Historic; just use `open`.
- **openat (295)** — `openat(dirfd, path, flags, mode)` in `ebx`,`ecx`,`edx`,`esi`: a relative `path` is resolved from directory fd `dirfd`; an absolute `path` (starting with `/`) ignores `dirfd`. Pass `AT_FDCWD` (-100) as `dirfd` to resolve relative paths from the current directory, exactly like `open`. `dirfd` is an fd for a directory, typically from `open(dir, O_RDONLY|O_DIRECTORY)`; `path`, `flags` and `mode` mean the same as for `open`. Modern libc uses this for everything.

## Example

```nasm
; nasm -f elf32 open.asm && i686-linux-gnu-ld -o open open.o && qemu-i386-static ./open
section .data
    fname db "out.txt", 0              ; paths must be NUL-terminated

section .text
global _start
_start:
    ; fd = open("out.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov eax, 5
    mov ebx, fname
    mov ecx, 0x241      ; 1 | 0x40 | 0x200
    mov edx, 0o644      ; rw-r--r--
    int 0x80
    test eax, eax
    js .fail            ; negative eax = -errno

    mov ebx, eax        ; close(fd)
    mov eax, 6
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
.fail:
    mov eax, 1
    mov ebx, 1          ; exit(1)
    int 0x80
```

## Linux philosophy

`open` is the gateway of "everything is a file": the same call opens regular files, devices (`/dev/tty`), and pseudo-files (`/proc/self/status`), and hands back the same kind of handle for all of them. That handle — a plain small integer — is what makes fds cheap to pass, duplicate, and inherit across `fork`/`exec`.

## In The Linux Programming Interface

Ch 4 §4.3 covers `open`, its flags, and file permissions; Ch 18 §18.11 explains `openat` and why "dirfd-relative" calls fix race conditions with changing directories.
