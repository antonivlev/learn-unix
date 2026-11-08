[← Back to index](README.md)

# uname (syscall 122)

Get the kernel's name, release, version, and machine architecture.

## What it does
`uname` fills a `struct utsname` you provide with identification strings: OS name ("Linux"), hostname, kernel release (e.g. "6.8.0-...") and version, machine ("i686"/"x86_64"), and domain name. It's what the `uname -a` command calls.

## How it works
```c
#include <sys/utsname.h>
int uname(struct utsname *buf);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 122 |
| `ebx` | `buf` | **Out.** Address of your memory; the kernel writes the whole struct there. Must have room for 390 bytes |

Syscall 122 fills the kernel's `struct new_utsname`: **6 fixed fields of 65 bytes each** (390 bytes total), each a NUL-terminated string padded with NULs. (The older numbers 109 and 59 fill smaller, outdated layouts.)

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 65 | `sysname` | OS name: "Linux" |
| 65 | 65 | `nodename` | Hostname of the machine (or of the container's UTS namespace) |
| 130 | 65 | `release` | Kernel release, e.g. "6.8.0-41-generic" |
| 195 | 65 | `version` | Build info, e.g. "#41-Ubuntu SMP PREEMPT_DYNAMIC ..." |
| 260 | 65 | `machine` | Hardware architecture, e.g. "i686" (a 64-bit kernel reports "x86_64") |
| 325 | 65 | `domainname` | NIS domain name, usually "(none)" |

Returns 0 on success and the strings are then in `buf`; `-errno` on error: `-EFAULT` (-14, bad buffer pointer).

## Example
```nasm
; nasm -f elf32 un.asm && i686-linux-gnu-ld -o un un.o && qemu-i386-static ./un
; Prints the kernel release string.
section .bss
    uts resb 390        ; 6 fields x 65 bytes

section .text
global _start
_start:
    ; uname(&uts)
    mov eax, 122
    mov ebx, uts
    int 0x80

    ; write(1, uts.release, 65)  -- release is at offset 130
    mov eax, 4
    mov ebx, 1
    lea ecx, [uts + 130]
    mov edx, 65         ; prints trailing NULs too; fine for a demo
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
Fixed-size, flat, string-filled structs like `utsname` are classic UNIX pragmatism: no allocation, no versioning dance, one cheap call. It also lets programs adapt to their environment at runtime (check kernel release before using a new feature) instead of being compiled blind.

## In The Linux Programming Interface
Ch 12 "System and Process Information" (p.223), §12.1. The book shows a complete C example and notes the same information (and much more) is browsable as files under `/proc/sys/kernel/`.
