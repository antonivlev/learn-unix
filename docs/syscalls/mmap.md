[← Back to index](README.md)

# mmap2 (syscall 192)

Map memory into your process — either a file, or fresh "anonymous" memory (this is how modern `malloc` gets big blocks).

## What it does
`mmap2` asks the kernel to add a new region to your address space. You can map a file (pass its fd in `edi`, so reading memory reads the file), or ask for *anonymous* memory with `MAP_ANONYMOUS` and fd -1: plain zero-filled pages, perfect as a heap replacement. On 32-bit x86, `mmap2` (192) is the modern call; old `mmap` (90) took a pointer to a struct of args and is best avoided. `munmap` (91) releases a region.

`addr` is a placement hint: 0 lets the kernel pick a free spot, and a nonzero address is used if it's free (otherwise the kernel picks elsewhere). With `MAP_FIXED` the mapping goes exactly at `addr`.

`prot` sets what the program may do with the pages (OR the bits together):

| Constant | Value | Meaning |
|---|---|---|
| `PROT_NONE` | 0 | No access; any touch raises `SIGSEGV` |
| `PROT_READ` | 1 | Pages can be read |
| `PROT_WRITE` | 2 | Pages can be written |
| `PROT_EXEC` | 4 | Pages can be executed as code |

`flags` picks the mapping type. Include exactly one of `MAP_SHARED` / `MAP_PRIVATE`, then OR in extras:

| Constant | Value | Meaning | Example `esi` |
|---|---|---|---|
| `MAP_SHARED` | 0x01 | Writes go to the underlying file and are seen by every process mapping it (shared anonymous memory is shared with children after `fork`) | 0x01: edit a file in place |
| `MAP_PRIVATE` | 0x02 | Copy-on-write: your writes stay in your private copy and never reach the file | 0x02: load a file read-mostly |
| `MAP_FIXED` | 0x10 | Place the mapping exactly at `addr` (must be page-aligned), replacing anything already mapped there | 0x12: private, at a chosen address |
| `MAP_ANONYMOUS` | 0x20 | No file: zero-filled memory; `fd` should be -1 and offset 0 | 0x22: heap-style allocation |

The offset in `ebp` counts **4096-byte pages**: to map a file starting at byte 8192, pass `ebp` = 2 (8192 / 4096). Byte offsets that aren't a multiple of 4096 can't be expressed.

## How it works
```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 192 |
| `ebx` | `addr` | **In.** Where you'd like the mapping to start. 0 lets the kernel choose (the usual case); a nonzero value is a hint, or the exact address with `MAP_FIXED`, where it must be page-aligned (a multiple of 4096) |
| `ecx` | `length` | **In.** Size of the mapping in bytes. Must be greater than 0; the kernel rounds it up to a whole number of 4096-byte pages |
| `edx` | `prot` | **In.** What the program may do with the pages: read, write, execute, or nothing. OR the bits from the table above, e.g. `PROT_READ`\|`PROT_WRITE` = 3 |
| `esi` | `flags` | **In.** Shared or private, file or anonymous, fixed address or not: OR the bits from the table above, e.g. `MAP_PRIVATE`\|`MAP_ANONYMOUS` = 0x22 |
| `edi` | `fd` | **In.** File descriptor of the file to map, from `open`. Use -1 with `MAP_ANONYMOUS` |
| `ebp` | `offset` | **In.** Where in the file the mapping starts, counted **in 4096-byte pages**: page 2 = byte 8192. Use 0 with `MAP_ANONYMOUS` |

Returns in `eax` the start address of the new mapping (page-aligned; with `MAP_FIXED` it equals `addr`). A value in the -1..-4095 range is `-errno`. Common errors: `-ENOMEM` (out of memory / too many mappings), `-EINVAL` (bad length or flags, or unaligned `addr` with `MAP_FIXED`), `-EBADF` (bad `fd` for a file mapping), `-EACCES` (the file's open mode doesn't allow `prot`).

`munmap` (91) removes a mapping: `int munmap(void *addr, size_t length);`

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 91 |
| `ebx` | `addr` | **In.** Start of the range to unmap, usually the address `mmap2` returned. Must be page-aligned (a multiple of 4096) |
| `ecx` | `length` | **In.** Number of bytes to unmap, rounded up to whole 4096-byte pages. You may unmap just part of a mapping |

`munmap` returns 0 on success; afterwards any access to the range raises `SIGSEGV`. Unmapping a range that holds no mapping is not an error. It returns `-EINVAL` (-22) for an unaligned `addr` or a `length` of 0.

The old `mmap` (90) takes a single argument: `ebx` points to six 4-byte values in memory, in the same order as the C prototype. Its offset is in **bytes** and must be a multiple of 4096. It returns the mapping address just like `mmap2`.

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `addr` | Requested start address, as for `mmap2` |
| 4 | 4 | `len` | Length in bytes |
| 8 | 4 | `prot` | Protection bits |
| 12 | 4 | `flags` | Mapping flags |
| 16 | 4 | `fd` | File descriptor, or -1 for anonymous |
| 20 | 4 | `offset` | File offset in bytes (page-aligned) |

## Example
```nasm
    ; ptr = mmap2(0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
    mov eax, 192
    xor ebx, ebx        ; addr = 0 (kernel picks)
    mov ecx, 4096       ; one page
    mov edx, 3          ; PROT_READ | PROT_WRITE
    mov esi, 0x22       ; MAP_PRIVATE | MAP_ANONYMOUS
    mov edi, -1         ; no file
    xor ebp, ebp        ; page offset 0 (PAGES, not bytes)
    int 0x80
    ; eax now holds a pointer to 4096 zeroed bytes

    mov byte [eax], 'A' ; use it like any memory

    ; munmap(ptr, 4096)
    mov ebx, eax
    mov eax, 91
    mov ecx, 4096
    int 0x80
```

## Linux philosophy
`mmap` unifies files and memory: the same call gives you a heap allocation or a file whose bytes appear directly in your address space, and `read`/`write` become plain loads and stores. It exposes the kernel's paging machinery as one small primitive that `malloc`, shared libraries, and databases are all built on.

## In The Linux Programming Interface
Chapter 49, "Memory Mappings" (p.1017). The book covers file vs anonymous and private vs shared mappings in depth, including using shared mappings for IPC and how copy-on-write makes `MAP_PRIVATE` cheap.
