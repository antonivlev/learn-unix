[← Back to index](README.md)

# socketcall (syscall 102)

The historical 32-bit single entry point for all socket operations.

## What it does
For most of Linux's history, 32-bit x86 had no separate `socket`/`bind`/`listen` syscalls. Instead there was one entry point, `socketcall`: you pass a *call number* saying which operation you want, plus a pointer to an array in memory holding the real arguments. Kernels >= 4.3 added direct syscalls (359+), which are easier — but `socketcall` still works everywhere and is what you'll see in older tutorials and `strace` output on old systems.

## How it works
```c
int socketcall(int call, unsigned long *args);   /* no glibc wrapper */
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 102 |
| `ebx` | `call` | **In.** Which socket operation to run, as a small integer (1 = `SYS_SOCKET`, 2 = `SYS_BIND`, …); see the table below |
| `ecx` | `args` | **In.** Address of an array in your memory holding the operation's real arguments, one 32-bit value (`dd`) each, in the same order as that operation's parameters. Pointer arguments (like `bind`'s `addr`) go in the array as addresses. Any **Out.** pointer among them is written by the kernel exactly as with the direct syscall |

Layout of the `args` array (4 bytes per slot, as many slots as the operation takes):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | arg 1 | First parameter of the operation (e.g. `sockfd`, or `domain` for `SYS_SOCKET`) |
| 4 | 4 | arg 2 | Second parameter (e.g. `addr` pointer for `SYS_BIND`) |
| 8 | 4 | arg 3 | Third parameter (e.g. `addrlen` = 16 for `SYS_BIND`) |
| 12, 16 | 4 each | arg 4, arg 5 | Further parameters, for operations that take them (`SYS_ACCEPT4`'s `flags`, `SYS_SETSOCKOPT`'s `optval`/`optlen`) |

For example `bind(fd, &addr, 16)` becomes `call` = 2 and `args` = `dd fd, addr, 16`.

The call number in `ebx` selects the operation and how many dwords `ecx` must point to:

| Call | Value | Operation | Args array (`ecx` points to) | Direct syscall (>= 4.3) |
|---|---|---|---|---|
| `SYS_SOCKET` | 1 | `socket` | `domain, type, protocol` | 359 |
| `SYS_BIND` | 2 | `bind` | `fd, addr_ptr, addrlen` | 361 |
| `SYS_CONNECT` | 3 | `connect` | `fd, addr_ptr, addrlen` | 362 |
| `SYS_LISTEN` | 4 | `listen` | `fd, backlog` | 363 |
| `SYS_ACCEPT` | 5 | `accept` | `fd, addr_ptr, addrlen_ptr` | none; use `accept4` 364 with flags 0 |
| `SYS_SEND` | 9 | `send` | `fd, buf, len, flags` | none; use `sendto` 369 with NULL addr |
| `SYS_RECV` | 10 | `recv` | `fd, buf, len, flags` | none; use `recvfrom` 371 with NULL addr |
| `SYS_SHUTDOWN` | 13 | `shutdown` | `fd, how` | 373 |
| `SYS_SETSOCKOPT` | 14 | `setsockopt` | `fd, level, optname, optval_ptr, optlen` | 366 |
| `SYS_ACCEPT4` | 18 | `accept4` | `fd, addr_ptr, addrlen_ptr, flags` | 364 |

Each slot means exactly what that parameter means in the direct call; see [socket](socket.md), [bind](bind.md), [connect](connect.md), [listen](listen.md), [accept4](accept4.md) (also for `accept`, which is `accept4` with flags 0); `setsockopt` is explained in [bind](bind.md). For the rest: `send`/`recv` take `fd` (**In.** a connected socket), `buf` (address of your data; **In.** for `send`, **Out.** for `recv`), `len` (**In.** bytes to send, or the most bytes to receive) and `flags` (**In.** 0 for normal behaviour); `shutdown` takes `fd` and `how` (**In.** 0 = stop reading, 1 = stop writing, 2 = both).

The return value in `eax` is whatever the underlying operation returns: a new socket fd for `SYS_SOCKET`, a new connected fd for `SYS_ACCEPT`/`SYS_ACCEPT4`, the byte count for `SYS_SEND`/`SYS_RECV`, 0 for the others; negative means `-errno`. Common errors: `-EINVAL` (bad call number), `-EFAULT` (bad args pointer). The classic bug: putting the arguments in registers instead of memory — they must be dwords in an array that `ecx` points to.

## Example
```nasm
section .data
    sockargs dd 2, 1, 0     ; AF_INET, SOCK_STREAM, protocol 0

section .text
    ; fd = socketcall(SYS_SOCKET, {2, 1, 0})
    mov eax, 102
    mov ebx, 1              ; SYS_SOCKET
    mov ecx, sockargs       ; pointer to args in MEMORY
    int 0x80
    ; eax = socket fd
```

Equivalent modern direct call: `eax=359, ebx=2, ecx=1, edx=0` — no memory array needed. Prefer that in your HTTP server if your kernel is >= 4.3.

## Linux philosophy
`socketcall` is a lesson in API evolution, not elegance: sockets came from BSD as a bundle, and Linux bolted them on through one door rather than many syscall numbers. Decades later the direct syscalls arrived — but the old entry point remains, because Linux never breaks existing programs.

## In The Linux Programming Interface
TLPI doesn't document `socketcall` itself — glibc hides it completely behind `socket()`, `bind()`, etc. For the actual socket operations behind it, see Chapter 56, "Sockets: Introduction" (p.1149).
