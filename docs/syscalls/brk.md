[← Back to index](README.md)

# brk (syscall 45)

Move the "program break" — the end of the process's heap — to grow or shrink it.

## What it does
The heap is a region that starts just after your program's data and ends at the **program break**. `brk` asks the kernel to move that boundary: raise it to get more usable memory, lower it to give memory back. This is (part of) what `malloc` does underneath — it grabs big chunks with `brk`/`mmap2` and hands out slices. The idiom: call with `ebx = 0` to query the current break, then call again with a higher address to claim the memory in between.

What happens depends on the address in `ebx` compared with the current break:

| `ebx` | Effect | Example |
|---|---|---|
| 0 (or any address below the start of the heap) | Nothing moves; returns the current break | `brk(0)` |
| above the current break | Grows the heap; the memory between the old and new break becomes usable | `brk(cur + 4096)` claims one more page |
| below the current break (and at or above the heap start) | Shrinks the heap; memory above the new break is given back and must no longer be touched | `brk(cur - 4096)` |

## How it works
```c
int brk(void *addr);            /* C wrapper: returns 0 / -1 */
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 45 |
| `ebx` | `addr` | **In.** The address you want the heap to end at (the first byte past the usable heap). Usually the current break plus or minus a number of bytes. Any byte address works; the kernel maps whole 4096-byte pages behind it. 0 only queries the current break (see the table above) |

Return in `eax`: **the program break after the call**, as an address. On success it equals the `addr` you passed; for `brk(0)` it is the current break. This differs from C's `brk()`, which returns 0/-1. On failure (e.g. out of memory), the raw syscall returns the *unchanged* break rather than an errno, so check whether `eax` matches what you asked for.

## Example
```nasm
; nasm -f elf32 brk.asm && i686-linux-gnu-ld -o brk brk.o && qemu-i386-static ./brk
section .text
global _start
_start:
    ; query current break: brk(0)
    mov eax, 45
    xor ebx, ebx
    int 0x80            ; eax = current break

    ; grow the heap by 4096 bytes
    mov esi, eax        ; save old break = start of our new memory
    lea ebx, [eax + 4096]
    mov eax, 45
    int 0x80            ; eax = new break (old + 4096 on success)

    ; use the memory
    mov byte [esi], 'A'     ; write into our fresh heap byte

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

```c
#include <unistd.h>
void *p = sbrk(4096);   /* libc helper: grow break, return old break */
```

## Linux philosophy
`brk` exposes the classic UNIX process memory model: one flat address space with a heap that grows upward from a single movable boundary. The kernel provides only this crude primitive; all the cleverness of `malloc` — free lists, chunk headers, reuse — is userspace policy layered on a minimal kernel mechanism. (Modern allocators also use `mmap2` for large blocks, precisely because one linear break can't return middle chunks.)

## In The Linux Programming Interface
Ch 7 "Memory Allocation" (p.139), §7.1. The book walks through `brk`/`sbrk`, then builds up to how `malloc` and `free` actually manage the heap on top of them.
