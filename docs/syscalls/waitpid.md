[← Back to index](README.md)

# waitpid (syscall 7) and wait4 (syscall 114)

Wait for a child process to exit and collect its exit status.

## What it does
When a child exits, it becomes a **zombie**: a dead process whose exit status the kernel keeps until the parent asks for it. `waitpid` blocks until a child changes state (usually: exits), collects the status, and lets the kernel free the zombie. If you never wait, zombies pile up in the process table. `wait4` is the same call plus an optional resource-usage struct — it's what libc's `waitpid()` actually invokes.

The `pid` argument chooses which children to wait for:

| `pid` | Waits for | Example |
|---|---|---|
| > 0 | the child with exactly that PID | `waitpid(1234, &st, 0)` |
| -1 | any child | `waitpid(-1, &st, 0)` reaps whichever child finishes first |
| 0 | any child in the caller's process group | `waitpid(0, &st, 0)` |
| < -1 | any child in process group `-pid` | `waitpid(-500, &st, 0)` waits on group 500 |

The `options` argument is a bitmask (OR the flags together; 0 = block until a child exits):

| Constant | Value | Meaning | Example |
|---|---|---|---|
| `WNOHANG` | 1 | return immediately with 0 if no child has changed state yet | `waitpid(-1, &st, WNOHANG)` polls for finished children |
| `WUNTRACED` | 2 | also return for children stopped by a signal (e.g. Ctrl-Z) | `waitpid(pid, &st, WUNTRACED)` |
| `WCONTINUED` | 8 | also return for stopped children resumed by `SIGCONT` | `waitpid(pid, &st, WCONTINUED)` |

## How it works
```c
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage);
```

| Register | `waitpid` (7) | `wait4` (114) |
|---|---|---|
| `eax` | 7 | 114 |
| `ebx` | pid (-1 = any child) | pid |
| `ecx` | status pointer (0 = don't store the status) | status pointer |
| `edx` | options (0 = block; `WNOHANG`=1, `WUNTRACED`=2, `WCONTINUED`=8) | options |
| `esi` | — | rusage pointer (0 = don't collect usage) |

| Parameter | Meaning |
|---|---|
| `pid` | **In.** Which child or children to wait for: a specific PID (as returned by `fork`), -1 for any child, or a process group — see the table above |
| `status` | **Out.** Address of a 4-byte integer in your memory; the kernel writes the child's status word there (exit code or signal — see the status encoding table below). 0 = don't store it; you still reap the child and get its PID |
| `options` | **In.** Bitmask that controls whether the call blocks and which state changes count; 0 = block until a child exits — see the table above |
| `rusage` | **Out.** (`wait4` only.) Address of a 72-byte `struct rusage`; the kernel fills it with the reaped child's resource usage (layout below). 0 = don't collect it |

`struct rusage` on i386 (all fields 4 bytes; the two times are `timeval` = {seconds, microseconds}):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 8 | `ru_utime` | CPU time spent running the child's own code: `tv_sec` at +0, `tv_usec` at +4 |
| 8 | 8 | `ru_stime` | CPU time the kernel spent working for the child (in syscalls), same format |
| 16 | 4 | `ru_maxrss` | peak resident memory, in kilobytes |
| 20 | 12 | `ru_ixrss`, `ru_idrss`, `ru_isrss` | unused on Linux (always 0) |
| 32 | 4 | `ru_minflt` | page faults served without disk I/O |
| 36 | 4 | `ru_majflt` | page faults that needed disk I/O |
| 40 | 4 | `ru_nswap` | unused (0) |
| 44 | 4 | `ru_inblock` | blocks read from the filesystem |
| 48 | 4 | `ru_oublock` | blocks written to the filesystem |
| 52 | 12 | `ru_msgsnd`, `ru_msgrcv`, `ru_nsignals` | unused (0) |
| 64 | 4 | `ru_nvcsw` | voluntary context switches (the child waited, e.g. for I/O) |
| 68 | 4 | `ru_nivcsw` | involuntary context switches (the scheduler preempted it) |

Return in `eax`: PID of the reaped child, 0 if `WNOHANG` was given and no child has changed state yet, or `-ECHILD` (-10) if there are no matching children, `-EINTR` (-4) if interrupted by a signal.

**Status encoding:** the low bits of the status word say why the child changed state:

| Child | Low 8 bits | Bits 8–15 | Decode with |
|---|---|---|---|
| exited normally | 0 | exit code — shift right by 8 to get it | `WIFEXITED`, `WEXITSTATUS` |
| killed by a signal | signal number (bit 7 = 0x80 set if it dumped core) | 0 | `WIFSIGNALED`, `WTERMSIG` |
| stopped (`WUNTRACED`) | 0x7f | stop signal number | `WIFSTOPPED`, `WSTOPSIG` |
| continued (`WCONTINUED`) | whole word is 0xffff | | `WIFCONTINUED` |

## Example
```nasm
; nasm -f elf32 wait.asm && i686-linux-gnu-ld -o wait wait.o && qemu-i386-static ./wait
section .bss
    status resd 1

section .text
global _start
_start:
    mov eax, 2          ; fork()
    int 0x80
    test eax, eax
    jnz .parent
    mov eax, 1          ; child: exit(7)
    mov ebx, 7
    int 0x80
.parent:
    mov ebx, eax        ; child PID
    mov eax, 7          ; waitpid(pid, &status, 0)
    mov ecx, status
    xor edx, edx
    int 0x80
    mov ebx, [status]
    shr ebx, 8          ; exit code is in bits 8-15
    and ebx, 0xff       ; ebx = 7
    mov eax, 1          ; exit with the child's code
    int 0x80
```

## Linux philosophy
A process's death is itself information: the kernel refuses to discard an exit status until someone reads it, which is exactly why zombies exist. Parent-child waiting is the synchronization half of fork/exec — the shell forks, execs your command, then waitpids to get `$?`.

## In The Linux Programming Interface
Ch 26 "Monitoring Child Processes" (p.541). The book covers the `WIFEXITED`/`WEXITSTATUS` macros that decode the status word, `SIGCHLD`, orphans (reparented to init), and zombies in depth.
