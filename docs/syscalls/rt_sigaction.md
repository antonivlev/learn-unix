[← Back to index](README.md)

# rt_sigaction (syscall 174)

Install, change, or inspect the handler for a signal.

## What it does
`rt_sigaction` tells the kernel what to do when a given signal arrives: run a handler function, ignore it (`SIG_IGN` = 1), or restore the default (`SIG_DFL` = 0). It's the syscall underneath libc's `signal()` and `sigaction()`.

The handler field of the action struct selects what happens:

| Handler | Value | On delivery | Example |
|---|---|---|---|
| `SIG_DFL` | 0 | the signal's default action (terminate, ignore, stop, ... — see the table in [kill](kill.md)) | restore normal Ctrl-C behaviour |
| `SIG_IGN` | 1 | the signal is discarded | ignore `SIGINT` so Ctrl-C does nothing |
| function address | any other value | the kernel calls `handler(signum)` (or `handler(signum, info, ucontext)` with `SA_SIGINFO`) | catch `SIGTERM` to clean up before exiting |

`SIGKILL` (9) and `SIGSTOP` (19) always keep their default action.

The flags field is a bitmask (OR them together; 0 = none). The commonly used ones:

| Constant | Value | Meaning |
|---|---|---|
| `SA_NOCLDSTOP` | 0x00000001 | for `SIGCHLD`: don't send it when a child merely stops |
| `SA_NOCLDWAIT` | 0x00000002 | for `SIGCHLD`: exited children are reaped automatically (no zombies) |
| `SA_SIGINFO` | 0x00000004 | call the handler with 3 arguments: signum, `siginfo_t *` (sender PID, fault address, ...), `ucontext *` |
| `SA_RESTORER` | 0x04000000 | the `restorer` field is valid (libc always sets this) |
| `SA_ONSTACK` | 0x08000000 | run the handler on the alternate stack set up with `sigaltstack` |
| `SA_RESTART` | 0x10000000 | automatically restart syscalls the signal interrupted (without it they fail with `-EINTR`) |
| `SA_NODEFER` | 0x40000000 | don't block this signal while its own handler runs |
| `SA_RESETHAND` | 0x80000000 | reset the handler to `SIG_DFL` after one delivery |

The mask field lists extra signals to block while the handler runs: bit *n*−1 stands for signal *n* (e.g. `SIGINT` = bit 1 = 0x2 in the first dword).

**Honest warning:** installing a real handler *function* via the raw syscall is tricky. When a handler finishes, it returns into a helper routine (the "restorer") that calls `rt_sigreturn` (173) to jump back to the interrupted code. The kernel uses the `restorer` field only when `sa_flags` includes `SA_RESTORER`; the C library always sets that flag and supplies its own routine. With `SA_RESTORER` clear, i386 falls back to a trampoline in the vDSO, while x86-64 has no fallback and requires your own restorer. In practice: use C's `sigaction()` for real handlers. The raw syscall is simplest for ignoring (`SIG_IGN`) or resetting (`SIG_DFL`) — those need no restorer at all.

## How it works
```c
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);   /* libc */
int rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact,
                 size_t sigsetsize);                                                  /* raw syscall */
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 174 |
| `ebx` | `signum` | **In.** The signal whose action you set or query, 1–64 (e.g. 2 = `SIGINT`; see the table in [kill](kill.md)). `SIGKILL` (9) and `SIGSTOP` (19) can be queried but not changed |
| `ecx` | `act` | **In.** Address of a 20-byte kernel `struct sigaction` describing the new action (layout below). 0 = leave the action unchanged and just query it |
| `edx` | `oldact` | **Out.** Address of a 20-byte buffer; the kernel writes the action that was in effect before this call there, in the same layout. 0 = don't report it |
| `esi` | `sigsetsize` | **In.** Size in bytes of the `sa_mask` field. **Must be 8** on x86-32 (64 signals, one bit each); any other value gives `-EINVAL` |

The kernel's `struct sigaction` on i386, which `act` and `oldact` point to:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `sa_handler` | `SIG_DFL` (0), `SIG_IGN` (1), or the address of your handler function — see the handler table above |
| 4 | 4 | `sa_flags` | `SA_*` bitmask; 0 = none — see the flags table above |
| 8 | 4 | `sa_restorer` | address of the code that calls `rt_sigreturn` after the handler returns; read only when `sa_flags` has `SA_RESTORER`, otherwise put 0 |
| 12 | 8 | `sa_mask` | signals to block while the handler runs: bit *n*−1 = signal *n*; first dword = signals 1–32, second = 33–64 |

This is the kernel's layout. glibc's C `struct sigaction` is different (handler, then a 128-byte `sa_mask`, then `sa_flags`, then `sa_restorer`) and libc converts it before making the syscall, so a C `struct sigaction` can't be passed to the raw syscall as-is.

Return in `eax`: 0 on success — the new action is in place, and `*oldact` holds the previous one if you asked for it. Errors: `-EINVAL` (-22) — bad signal number, wrong sigsetsize, or trying to change the action of SIGKILL/SIGSTOP; `-EFAULT` (-14) — `act` or `oldact` points to invalid memory.

## Example
Ignore Ctrl-C (SIGINT) for 5 seconds — no restorer needed for `SIG_IGN`.

```nasm
; nasm -f elf32 sigign.asm && i686-linux-gnu-ld -o sigign sigign.o && qemu-i386-static ./sigign
section .data
    act  dd 1, 0, 0, 0, 0   ; handler=SIG_IGN, flags=0, restorer=0, mask (8 bytes)
    ts   dd 5, 0            ; timespec {5 s, 0 ns}

section .text
global _start
_start:
    mov eax, 174        ; rt_sigaction(SIGINT, &act, NULL, 8)
    mov ebx, 2          ; SIGINT
    mov ecx, act
    xor edx, edx
    mov esi, 8          ; sigsetsize = 8
    int 0x80

    mov eax, 162        ; nanosleep(&ts, NULL) -- try Ctrl-C, nothing happens
    mov ebx, ts
    xor ecx, ecx
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
A process doesn't poll for signals — it declares in advance how each one should be handled, and the kernel remembers. The awkward restorer detail shows a real design choice: returning from a signal handler needs helper code, and Linux makes the program (in practice, the C library) supply it rather than hardwiring it into the kernel.

## In The Linux Programming Interface
Ch 20 §20.13 and Ch 21 "Signals: Signal Handlers" (p.421). The book covers the `sigaction` struct fields, `SA_RESTART` and friends, and which functions are safe to call from inside a handler.
