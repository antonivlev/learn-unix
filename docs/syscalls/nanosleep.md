[← Back to index](README.md)

# nanosleep (syscall 162)

Pause the calling thread for a given time span, with nanosecond precision.

## What it does
`nanosleep` puts the process to sleep for at least the requested duration (the kernel may round up to its timer resolution). The duration is a `struct timespec`: on x86-32 that's simply **two 32-bit words** — seconds, then nanoseconds (0–999,999,999). It's what libc's `sleep()` and `usleep()` are built on.

## How it works
```c
int nanosleep(const struct timespec *req, struct timespec *rem);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 162 |
| `ebx` | `req` | **In.** Address of an 8-byte `struct timespec` (layout below) holding how long to sleep. The kernel only reads it |
| `ecx` | `rem` | **Out.** Address of another 8-byte `struct timespec`; if a signal cuts the sleep short, the kernel writes the time still left there. 0 (NULL) if you don't need it. It may point at the same struct as `req`, which makes resuming a one-liner |

Both structs have the same i386 layout:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `tv_sec` | Whole seconds; must be 0 or more |
| 4 | 4 | `tv_nsec` | Extra nanoseconds, 0–999,999,999 (500,000,000 = half a second) |

Return in `eax`: 0 after sleeping the full time. If a signal interrupts the sleep, returns `-EINTR` (-4) and, when `ecx` is non-zero, writes the **unslept remainder** into the `ecx` struct, so you can loop and resume. `-EINVAL` (-22) if nsec is outside 0–999,999,999 or sec is negative.

## Example
```nasm
; nasm -f elf32 sleep.asm && i686-linux-gnu-ld -o sleep sleep.o && qemu-i386-static ./sleep
; sleeps 1.5 seconds, then prints "done"
section .data
    ts  dd 1, 500000000     ; timespec: 1 s + 500,000,000 ns
    msg db "done", 10

section .bss
    rem resd 2              ; remainder written here if interrupted

section .text
global _start
_start:
    mov eax, 162        ; nanosleep(&ts, &rem)
    mov ebx, ts
    mov ecx, rem
    int 0x80
    ; eax = -4 (-EINTR) here would mean a signal woke us early

    mov eax, 4          ; write(1, msg, 5)
    mov ebx, 1
    mov ecx, msg
    mov edx, 5
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
The remainder argument shows a core UNIX rule: signals can interrupt any slow syscall, and the kernel won't hide that from you — instead it hands back exactly how much work is left and lets userspace decide whether to resume. Time itself is just plain data (two integers in memory), passed by pointer like everything else.

## In The Linux Programming Interface
Ch 23 "Timers and Sleeping" (p.479), §23.4. The book covers the subtlety that restarting with the remainder drifts on repeated interruption, and the `clock_nanosleep` absolute-deadline fix.
