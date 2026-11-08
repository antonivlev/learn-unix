[← Back to index](README.md)

# futex (syscall 240)

"Fast userspace mutex" — the kernel primitive that pthread mutexes and condition variables are built on.

## What it does
A futex is just a normal 32-bit integer in your process's memory. The trick: when no other thread is competing (the common case), a lock is taken and released with **ordinary atomic CPU instructions — no syscall at all**. The kernel is involved only on contention: `FUTEX_WAIT` (op 0) puts the caller to sleep *if the integer still holds an expected value*, and `FUTEX_WAKE` (op 1) wakes threads sleeping on that address. You won't call this directly as a beginner — pthreads does — but it's why `strace` on a threaded program shows `futex(...)` everywhere.

The `op` argument selects the operation; `val` means something different for each:

| `op` | Value | Meaning | Example |
|---|---|---|---|
| `FUTEX_WAIT` | 0 | if `*uaddr == val`, sleep until woken (or until `timeout`); otherwise return `-EAGAIN` at once | `futex(&f, FUTEX_WAIT, 1, NULL)` sleeps while the lock is held |
| `FUTEX_WAKE` | 1 | wake up to `val` threads sleeping on `uaddr` | `futex(&f, FUTEX_WAKE, 1)` wakes one; `val` = `INT_MAX` wakes all |
| `FUTEX_PRIVATE_FLAG` | 128 | OR into `op`: the futex is used only by threads of this process, which lets the kernel skip shared-memory bookkeeping | `FUTEX_WAIT_PRIVATE` = 128, `FUTEX_WAKE_PRIVATE` = 129 (what glibc uses for ordinary mutexes) |

Other ops (`FUTEX_REQUEUE` = 3, `FUTEX_CMP_REQUEUE` = 4, `FUTEX_WAKE_OP` = 5, the `_PI` and `_BITSET` variants) exist for condition variables and priority inheritance; they use `uaddr2` and `val3`.

## How it works
```c
long futex(uint32_t *uaddr, int op, uint32_t val,
           const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 240 |
| `ebx` | `uaddr` | **In.** Address of the futex: a 4-byte integer in your memory (e.g. `fut resd 1`) that your threads also update with atomic instructions. Must be 4-byte aligned, otherwise `-EINVAL`. The kernel reads it for WAIT and never writes it; the address itself identifies the wait queue |
| `ecx` | `op` | **In.** Which operation to perform, optionally ORed with `FUTEX_PRIVATE_FLAG` (128) — see the table above |
| `edx` | `val` | **In.** Depends on `op`. WAIT: the value you expect `*uaddr` to hold; the kernel sleeps only if it still matches. WAKE: the maximum number of threads to wake (1 = one, `INT_MAX` = all) |
| `esi` | `timeout` | **In.** WAIT: address of a `timespec` (layout below) giving the longest time to sleep, relative to now; 0 = wait forever. WAKE ignores it. The requeue ops reuse this slot as a plain number (`val2`) |
| `edi` | `uaddr2` | **In.** Address of a second futex, used only by `FUTEX_REQUEUE`, `FUTEX_CMP_REQUEUE` and `FUTEX_WAKE_OP`; pass 0 for WAIT and WAKE |
| `ebp` | `val3` | **In.** Extra value for `FUTEX_CMP_REQUEUE`, `FUTEX_WAKE_OP` and the `_BITSET` ops; pass 0 for WAIT and WAKE |

The `timespec` that `timeout` points to (i386, 32-bit fields):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `tv_sec` | whole seconds |
| 4 | 4 | `tv_nsec` | extra nanoseconds, 0–999,999,999 (larger values give `-EINVAL`) |

For example `ts dd 2, 500000000` is 2.5 seconds. (Syscall 422, `futex_time64`, is the same call with 64-bit `tv_sec` and `tv_nsec` fields.)

Return in `eax`: WAKE returns the number of threads it woke (0 if nobody was waiting); WAIT returns 0 after being woken by a WAKE. Treat a 0 from WAIT as "go check the integer again", since a wakeup doesn't guarantee the lock is free. Errors: `-EAGAIN` (-11) — WAIT found `*uaddr != val` (someone changed it first, no sleep needed); `-ETIMEDOUT` (-110) — WAIT's timeout expired; `-EINTR` (-4) — interrupted by a signal.

## Example
Conceptually, a mutex lock looks like:
```
lock:   atomically try to change the int 0 -> 1 (e.g. lock cmpxchg)
        if it worked: done, no syscall           <- fast path
        else: futex(FUTEX_WAIT, expect 1) and retry  <- slow path
unlock: atomically set the int to 0
        if anyone might be sleeping: futex(FUTEX_WAKE, 1)
```

A harmless real call — wake up to 1 waiter on a futex nobody waits on (returns 0):
```nasm
section .bss
    fut resd 1
    ; ...
    mov eax, 240        ; futex(&fut, FUTEX_WAKE, 1, NULL)
    mov ebx, fut
    mov ecx, 1          ; FUTEX_WAKE
    mov edx, 1          ; wake at most 1
    xor esi, esi
    int 0x80            ; eax = 0 (nobody was waiting)
```

## Linux philosophy
Futexes show Linux's obsession with keeping the kernel off the fast path: syscalls are expensive, so the design ensures a well-behaved lock costs *zero* kernel entries, paying that cost only when threads actually collide. The compare-before-sleeping rule ("sleep only if the value is still what I expect") is how the kernel closes the race between checking a lock and going to sleep on it.

## In The Linux Programming Interface
TLPI covers the pthreads layer built on top in Ch 30 "Threads: Thread Synchronization" (p.631) — mutexes and condition variables, their semantics and pitfalls. Read that for what you'd actually use; futex is the machinery underneath.
