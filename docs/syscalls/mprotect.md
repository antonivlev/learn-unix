[← Back to index](README.md)

# mprotect (syscall 125)

Change the read/write/execute permissions of a region of memory you already own.

## What it does
Every page of memory has protection bits. `mprotect` lets you change them after the fact: make a buffer read-only, make a page executable (JIT compilers do this), or remove all access to create a "guard page" that crashes on touch instead of silently corrupting data. The region should come from `mmap2` and the address must be page-aligned (multiple of 4096).

The `prot` argument replaces the region's old permissions entirely; OR the bits together:

| Constant | Value | Meaning | Example |
|---|---|---|---|
| `PROT_NONE` | 0 | No access at all; any touch raises `SIGSEGV` | `mprotect(p, 4096, PROT_NONE)` makes a guard page |
| `PROT_READ` | 1 | Pages can be read | `mprotect(p, 4096, PROT_READ)` makes data read-only |
| `PROT_WRITE` | 2 | Pages can be written (x86 has no write-only pages, so this also allows reads) | `mprotect(p, 4096, PROT_READ \| PROT_WRITE)` (prot = 3) for normal data |
| `PROT_EXEC` | 4 | Pages can be executed as code | `mprotect(p, 4096, PROT_READ \| PROT_EXEC)` (prot = 5) for freshly JIT-compiled code |

## How it works
```c
int mprotect(void *addr, size_t len, int prot);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 125 |
| `ebx` | `addr` | **In.** Start of the region to change, e.g. an address returned by `mmap2`. Must be page-aligned (a multiple of 4096) |
| `ecx` | `len` | **In.** Size of the region in bytes, rounded up to whole 4096-byte pages. Every page in the range must already be mapped. 0 changes nothing and succeeds |
| `edx` | `prot` | **In.** The new permissions for every page in the range; OR the bits from the table above |

Returns 0 in `eax` on success; the new permissions take effect immediately. Common errors: `-EINVAL` (-22, addr not page-aligned or unknown prot bits), `-ENOMEM` (-12, part of the range isn't mapped), `-EACCES` (-13, e.g. `PROT_WRITE` on a shared mapping of a file opened read-only).

## Example
```nasm
    ; assume eax = pointer from mmap2(0, 8192, 3, 0x22, -1, 0)
    mov ebx, eax        ; addr of first page
    add ebx, 4096       ; second page
    mov eax, 125        ; mprotect
    mov ecx, 4096       ; one page
    xor edx, edx        ; PROT_NONE — guard page
    int 0x80
    ; any access to that page now delivers SIGSEGV
```

```c
mprotect(page, 4096, PROT_READ);            /* freeze data */
mprotect(code, 4096, PROT_READ | PROT_EXEC); /* never writable and executable at once */
```

## Linux philosophy
The kernel exposes the raw permission bits and lets programs build their own protections: guard pages under thread stacks, or the write-or-execute rule (see [glossary](../glossary.md)) that stops attacker data from being run as code. One tiny call, many security patterns.

## In The Linux Programming Interface
Chapter 50, "Virtual Memory Operations" (p.1045), §50.1. The book shows a full example that changes protection and demonstrates the resulting `SIGSEGV`, plus how to inspect a process's mappings via `/proc/PID/maps`.
