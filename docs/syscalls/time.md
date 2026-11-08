[← Back to index](README.md)

# time (13) / gettimeofday (78) / clock_gettime (265)

Three generations of "what time is it?" — use `clock_gettime`.

## What it does
All three report time as seconds since the **Unix epoch** (1970-01-01 UTC). `time` gives whole seconds; `gettimeofday` adds microseconds; `clock_gettime` adds nanoseconds *and* lets you pick a clock with its first argument. Prefer `clock_gettime` in new code.

| Clock id | Value | Measures | Example |
|---|---|---|---|
| `CLOCK_REALTIME` | 0 | Wall-clock time since the epoch; can jump when the clock is adjusted | `clock_gettime(CLOCK_REALTIME, &ts)` for timestamps |
| `CLOCK_MONOTONIC` | 1 | Time since an arbitrary point (roughly boot); only moves forward, pauses during suspend | `clock_gettime(CLOCK_MONOTONIC, &ts)` for measuring durations |
| `CLOCK_PROCESS_CPUTIME_ID` | 2 | CPU time used by this process | profiling a whole program |
| `CLOCK_THREAD_CPUTIME_ID` | 3 | CPU time used by this thread | profiling one thread |
| `CLOCK_BOOTTIME` | 7 | Like `CLOCK_MONOTONIC`, but keeps counting during suspend | uptime including sleep |

## How it works
```c
time_t time(time_t *t);                              /* or t = NULL */
int gettimeofday(struct timeval *tv, void *tz);      /* tz: pass NULL */
int clock_gettime(clockid_t clk, struct timespec *ts);
```

| Register | time (13) | gettimeofday (78) | clock_gettime (265) |
|---|---|---|---|
| `eax` | 13 | 78 | 265 |
| `ebx` | time_t* or 0 | timeval* | clock id (see table above) |
| `ecx` | — | 0 (tz, obsolete) | timespec* |

| Parameter | Meaning |
|---|---|
| `t` (time) | **Out.** Address of a 4-byte variable; the kernel stores the seconds since the epoch there as well as returning them. 0 (NULL) skips the store, which is the usual choice |
| `tv` (gettimeofday) | **Out.** Address of an 8-byte `struct timeval` (layout below); the kernel fills it with the current wall-clock time |
| `tz` (gettimeofday) | **Out.** Address of an old `struct timezone` (two 4-byte ints, minutes west of UTC and a DST flag) that the kernel fills from an old setting that is rarely meaningful. Pass 0 (NULL); real timezone handling lives in libc |
| `clk` (clock_gettime) | **In.** Which clock to read: wall clock, monotonic, CPU time, etc. See the table above |
| `ts` (clock_gettime) | **Out.** Address of an 8-byte `struct timespec` (layout below); the kernel fills it with the chosen clock's current value |

On x86-32 both structs are **two 32-bit words**:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `tv_sec` | Whole seconds (since the epoch for `CLOCK_REALTIME` and `gettimeofday`) |
| 4 | 4 | `tv_nsec` (timespec) / `tv_usec` (timeval) | Fraction of a second: nanoseconds 0–999,999,999 in a `timespec`, microseconds 0–999,999 in a `timeval` |

`time` returns the seconds since the epoch directly in `eax`. `gettimeofday` and `clock_gettime` return 0 on success and deliver the time through the struct. Errors: `-EFAULT` (-14, bad pointer), `-EINVAL` (-22, bad clockid).

**Year 2038:** 32-bit seconds overflow in 2038. `clock_gettime64` (403) takes the same `clk` but a 16-byte timespec (8-byte `tv_sec` at offset 0, 8-byte `tv_nsec` at offset 8) and is the long-term answer on 32-bit systems.

## Example
```nasm
; nasm -f elf32 t.asm && i686-linux-gnu-ld -o t t.o && qemu-i386-static ./t ; echo $?
section .bss
    ts resd 2           ; timespec: [seconds, nanoseconds]

section .text
global _start
_start:
    ; clock_gettime(CLOCK_MONOTONIC, &ts)
    mov eax, 265
    mov ebx, 1          ; CLOCK_MONOTONIC
    mov ecx, ts
    int 0x80

    ; time(NULL) -> seconds since epoch in eax
    mov eax, 13
    xor ebx, ebx
    int 0x80

    ; exit(seconds & 0xff) -- just to observe something
    mov ebx, eax
    and ebx, 0xff
    mov eax, 1
    int 0x80
```

```c
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);   /* ts.tv_sec, ts.tv_nsec */
```

## Linux philosophy
Counting seconds from a single epoch, in UTC, with zero formatting is the kernel keeping its interface minimal: one integer everyone agrees on. Timezones, calendars, and string formatting are userspace's job (libc, `/etc/localtime`). The progression time → gettimeofday → clock_gettime also shows how UNIX evolves: old calls are never removed, just superseded.

## In The Linux Programming Interface
Ch 10 "Time" (p.185) covers the epoch, `time`/`gettimeofday`, and calendar-time conversion; `clock_gettime` and the different clock types are in Ch 23 "Timers and Sleeping" (p.479), §23.5.
