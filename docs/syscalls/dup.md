[← Back to index](README.md)

# dup / dup2 (syscalls 41 / 63)

Duplicate a file descriptor — the mechanism behind shell redirection.

## What it does

`dup` makes a copy of an fd, returning the lowest free fd number. `dup2` copies it onto a *specific* number, silently closing whatever was there first. Both fds then share one open file description: same offset, same file status flags (`O_APPEND`, `O_NONBLOCK`). The close-on-exec flag belongs to each fd separately, and the new fd always starts with it off. The classic use: open a file, `dup2` it onto fd 1, and everything the program writes to "stdout" lands in the file — that's exactly how `prog > out.txt` works in the shell.

## How it works

```c
int dup(int oldfd);
int dup2(int oldfd, int newfd);
```

| Register | dup (41) | dup2 (63) |
|---|---|---|
| `eax` | 41 | 63 |
| `ebx` | oldfd | oldfd |
| `ecx` | — | newfd |

| Parameter | Meaning |
|---|---|
| `oldfd` | **In.** The open file descriptor to copy, e.g. a number returned by `open` or `pipe`. It stays open and keeps working |
| `newfd` | **In.** (`dup2` only) The exact fd number you want the copy to have, e.g. 0, 1 or 2 to replace stdin, stdout or stderr. Must be ≥ 0 and below the fd limit; see the table below for what happens if it's already open |

What `dup2` does depends on `newfd`:

| `newfd` | Result | Example |
|---|---|---|
| free | `oldfd` is copied onto `newfd` | `dup2(fd, 9)` with fd 9 unused |
| already open | `newfd` is closed first (errors from that close are ignored), then `oldfd` is copied onto it | `dup2(fd, 1)` replaces stdout |
| equal to `oldfd` | nothing happens; returns `newfd` (if `oldfd` is valid) | `dup2(3, 3)` returns 3 |

Return in `eax`: the new fd, which now refers to the same open file as `oldfd` (for `dup` the lowest free number, for `dup2` always `newfd`), or negative `-errno`.

Common errors: `-EBADF` (-9, oldfd isn't open, or newfd is negative or above the fd limit), `-EMFILE` (-24, out of fds).

Also: **dup3 (330)** — `dup3(oldfd, newfd, flags)` in `ebx`,`ecx`,`edx` — like `dup2` with a `flags` argument. `oldfd` and `newfd` mean the same as for `dup2`. Its only flag is `O_CLOEXEC` (0x80000), which sets close-on-exec on `newfd`; `flags` = 0 behaves like `dup2`. It returns `-EINVAL` (-22) if `oldfd == newfd`.

## Example

```nasm
; nasm -f elf32 redir.asm && i686-linux-gnu-ld -o redir redir.o && qemu-i386-static ./redir
; Redirects stdout to a file, then "prints" -- text lands in out.txt.
section .data
    fname  db "out.txt", 0
    msg    db "went to the file, not the screen", 10
    msglen equ $ - msg

section .text
global _start
_start:
    mov eax, 5          ; fd = open("out.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov ebx, fname
    mov ecx, 0x241
    mov edx, 0o644
    int 0x80

    mov ebx, eax        ; dup2(fd, 1): fd 1 now points at out.txt
    mov ecx, 1
    mov eax, 63
    int 0x80

    mov eax, 4          ; write(1, msg, msglen) -- goes to the file
    mov ebx, 1
    mov ecx, msg
    mov edx, msglen
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy

Redirection needs no support from the redirected program: because fds 0/1/2 are just conventions and fds survive `fork`/`exec`, the *shell* rewires them with `dup2` before `exec`, and the program writes to "fd 1" none the wiser. Small primitives (open, dup2, fork, exec) compose into pipelines and redirection — policy lives in the shell, mechanism in the kernel.

## In The Linux Programming Interface

Ch 5 "File I/O: Further Details" (p.89), §5.5. The book explains the fd table vs. the open file description (why duplicated fds share an offset) and shows the fork+dup2+exec redirection pattern in full.
