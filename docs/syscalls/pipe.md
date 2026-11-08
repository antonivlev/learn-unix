[← Back to index](README.md)

# pipe (syscall 42)

Create a one-way data channel between processes: bytes written to one end come out the other.

## What it does
`pipe` fills a 2-element int array with two new file descriptors: `fds[0]` is the **read end**, `fds[1]` is the **write end**. Whatever you `write` to `fds[1]` can be `read` from `fds[0]`. Combined with `fork` (a child inherits the fds) and `dup2`, this is exactly how the shell implements `ls | wc`. `pipe2` (331) is the same plus a flags argument (OR the bits together; 0 behaves exactly like `pipe`):

| Constant | Value | Meaning | Example |
|---|---|---|---|
| `O_NONBLOCK` | 0x800 | Reads on an empty pipe and writes to a full pipe return `-EAGAIN` (-11) instead of blocking | `pipe2(fds, O_NONBLOCK)` for an event loop |
| `O_CLOEXEC` | 0x80000 | Both fds close automatically on `execve`, so they don't leak into programs you launch | `pipe2(fds, O_CLOEXEC)` |
| `O_DIRECT` | 0x4000 | "Packet mode": each `write` becomes a separate message that a single `read` returns whole (Linux 3.4+) | `pipe2(fds, O_DIRECT)` |

## How it works
```c
int pipe(int fds[2]);
int pipe2(int fds[2], int flags);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 42 (`pipe`) or 331 (`pipe2`) |
| `ebx` | `fds` | **Out.** Address of 8 bytes of your memory (two 4-byte ints); the kernel writes the two new file descriptors there (layout below) |
| `ecx` | `flags` | **In.** `pipe2` only. Options for both new fds: non-blocking, close-on-exec, packet mode. OR the bits from the table above; 0 gives plain `pipe` behaviour |

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `fds[0]` | **Read end**: pass it to `read` |
| 4 | 4 | `fds[1]` | **Write end**: pass it to `write` |

Returns 0 in `eax` on success; the fds themselves arrive in your array. The kernel picks the lowest free fd numbers, e.g. 3 and 4 in a program that only has stdin, stdout and stderr open. Common errors: `-EMFILE` (-24, too many open fds), `-ENFILE` (-23, system-wide file limit reached), `-EFAULT` (-14, bad pointer), `-EINVAL` (-22, unknown `pipe2` flags). Without `O_NONBLOCK`, reading an empty pipe blocks (and writing to a full one blocks too); writing to a pipe whose read end is closed raises `SIGPIPE`.

## Example
```nasm
section .bss
    fds resd 2          ; fds[0]=read end, fds[1]=write end

section .text
    ; pipe(fds)
    mov eax, 42
    mov ebx, fds
    int 0x80

    ; write(fds[1], msg, len)
    mov eax, 4
    mov ebx, [fds + 4]  ; write end
    mov ecx, msg
    mov edx, msglen
    int 0x80

    ; read(fds[0], buf, 64)  — gets the same bytes back
    mov eax, 3
    mov ebx, [fds]      ; read end
    mov ecx, buf
    mov edx, 64
    int 0x80
```

The classic shell-pipeline pattern in C:
```c
int fds[2]; pipe(fds);
if (fork() == 0) {           /* child = left side of "|" */
    dup2(fds[1], 1);         /* stdout -> pipe write end */
    close(fds[0]); close(fds[1]);
    execve("/bin/ls", ...);
}
```

## Linux philosophy
The pipe is *the* UNIX idea: both ends are ordinary file descriptors, so `read`, `write`, `close`, and `dup2` all just work, and programs that only know stdin/stdout can be chained into pipelines without knowing pipes exist. Small programs composed through a byte stream.

## In The Linux Programming Interface
Chapter 44, "Pipes and FIFOs" (p.889). The book walks through building a shell pipeline step by step, why you must close unused ends, and FIFOs (named pipes that live in the filesystem).
