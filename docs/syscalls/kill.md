[← Back to index](README.md)

# kill (syscall 37)

Send a signal to a process.

## What it does
Despite the name, `kill` just delivers a **signal** — a small numbered notification — to another process (or to yourself). Many signals do kill by default, hence the name. Common ones: `SIGTERM` (15) politely asks a process to exit, `SIGKILL` (9) destroys it uncatchably, `SIGINT` (2) is what Ctrl-C sends. Signal 0 sends nothing but still runs the permission and existence checks — a standard trick to test "is this PID alive?".

Common `sig` values (x86 numbering):

| Constant | Value | Default action | Typical use |
|---|---|---|---|
| (none) | 0 | nothing sent | `kill(pid, 0)` checks the PID exists and you may signal it |
| `SIGHUP` | 1 | terminate | terminal closed; daemons often reload config |
| `SIGINT` | 2 | terminate | Ctrl-C |
| `SIGQUIT` | 3 | terminate + core dump | Ctrl-\ |
| `SIGKILL` | 9 | terminate (can't be caught or ignored) | `kill(pid, SIGKILL)` force-kills |
| `SIGUSR1` | 10 | terminate | free for your program's own meaning |
| `SIGSEGV` | 11 | terminate + core dump | sent by the kernel on a bad memory access |
| `SIGUSR2` | 12 | terminate | free for your program's own meaning |
| `SIGPIPE` | 13 | terminate | writing to a pipe with no reader |
| `SIGTERM` | 15 | terminate | `kill(pid, SIGTERM)` asks politely; the `kill` command's default |
| `SIGCHLD` | 17 | ignore | a child stopped or exited |
| `SIGCONT` | 18 | continue if stopped | resume a stopped process |
| `SIGSTOP` | 19 | stop (can't be caught or ignored) | pause a process |
| `SIGTSTP` | 20 | stop | Ctrl-Z |

The `pid` argument chooses who receives the signal:

| `pid` | Signal goes to | Example |
|---|---|---|
| > 0 | the process with that PID | `kill(1234, SIGTERM)` |
| 0 | every process in the caller's process group | `kill(0, SIGTERM)` |
| -1 | every process you have permission to signal, except init (PID 1) and yourself | `kill(-1, SIGKILL)` |
| < -1 | every process in process group `-pid` | `kill(-500, SIGINT)` signals group 500 |

## How it works
```c
int kill(pid_t pid, int sig);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 37 |
| `ebx` | `pid` | **In.** Who receives the signal: a process ID (from `fork`, `getpid`, `ps`, ...), or 0, -1 or a negative process-group number to signal many processes at once — see the `pid` table above |
| `ecx` | `sig` | **In.** Signal number to send, 1–64; 0 sends nothing and only checks that the target exists and you may signal it — see the signal table above |

Return in `eax`: 0 on success, meaning the signal was sent (to at least one process when `pid` selects a group); the call does not wait for the target to react. Errors: `-ESRCH` (-3) — no such process; `-EPERM` (-1) — you don't own that process; `-EINVAL` (-22) — invalid signal number.

## Example
```nasm
; nasm -f elf32 kill.asm && i686-linux-gnu-ld -o kill kill.o && qemu-i386-static ./kill
; sends SIGTERM to itself; shell reports "terminated"
section .text
global _start
_start:
    mov eax, 20         ; getpid()
    int 0x80

    mov ebx, eax        ; kill(pid, SIGTERM)
    mov eax, 37
    mov ecx, 15         ; SIGTERM
    int 0x80

    mov eax, 1          ; not reached (SIGTERM's default action kills us)
    xor ebx, ebx
    int 0x80
```

```c
#include <signal.h>
if (kill(pid, 0) == 0) puts("process exists");  /* signal 0 = probe */
```

## Linux philosophy
Signals are UNIX's software interrupts: asynchronous, one integer of information, delivered whether or not the target is expecting them. `kill` makes process control uniform — the same mechanism serves Ctrl-C from a terminal, a `kill` command from a shell, and the kernel reporting a segfault (`SIGSEGV`). The escalation ritual "try SIGTERM, then SIGKILL" reflects the design: processes may catch and clean up on most signals, but SIGKILL is the kernel's non-negotiable override.

## In The Linux Programming Interface
Ch 20 "Signals: Fundamental Concepts" (p.387), §20.5. The book covers the permission rules for sending signals, the full default-action table, and the signal-0 existence-check idiom.
