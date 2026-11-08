[← Back to index](README.md)

# socket (syscall 359)

Create a communication endpoint and get back an ordinary file descriptor.

## What it does
`socket` asks the kernel for a network endpoint. Three arguments pick what kind. The fd it returns works with `read`, `write`, and `close` like any file. This direct syscall number (359) exists on kernels >= 4.3; older 32-bit code went through `socketcall` (102).

`domain` picks the address family, which fixes the address format you later pass to `bind`/`connect`:

| `domain` | Value | Meaning | Example |
|---|---|---|---|
| `AF_UNIX` | 1 | Local socket between processes on the same machine, addressed by a filesystem path | `socket(AF_UNIX, SOCK_STREAM, 0)` |
| `AF_INET` | 2 | IPv4, addressed by `struct sockaddr_in` (IP + port) | `socket(AF_INET, SOCK_STREAM, 0)` for a TCP server |
| `AF_INET6` | 10 | IPv6, addressed by `struct sockaddr_in6` | `socket(AF_INET6, SOCK_STREAM, 0)` |

`type` picks the communication style. You can OR flags into it to set options on the new fd in the same call:

| `type` | Value | Meaning | Example |
|---|---|---|---|
| `SOCK_STREAM` | 1 | Reliable, ordered, connected byte stream (TCP in `AF_INET`) | `socket(AF_INET, SOCK_STREAM, 0)` |
| `SOCK_DGRAM` | 2 | Separate messages, no connection, may be lost or reordered (UDP in `AF_INET`) | `socket(AF_INET, SOCK_DGRAM, 0)` |
| `SOCK_NONBLOCK` | 0x800 | Flag: calls on the fd return `-EAGAIN` when they would otherwise wait | `socket(AF_INET, SOCK_STREAM\|SOCK_NONBLOCK, 0)` = type 0x801 |
| `SOCK_CLOEXEC` | 0x80000 | Flag: the fd is closed automatically on `execve` | `socket(AF_INET, SOCK_STREAM\|SOCK_CLOEXEC, 0)` = type 0x80001 |

`protocol` picks a specific protocol within the family and type. 0 selects the default (TCP for `AF_INET`+`SOCK_STREAM`, UDP for `AF_INET`+`SOCK_DGRAM`); you can also name it explicitly with `IPPROTO_TCP`=6 or `IPPROTO_UDP`=17.

## How it works
```c
int socket(int domain, int type, int protocol);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 359 |
| `ebx` | `domain` | **In.** Address family, a small integer constant. It decides which address struct `bind`/`connect` expect later; see the `domain` table above (2 = `AF_INET` for IPv4) |
| `ecx` | `type` | **In.** Communication style, optionally ORed with flag bits for the new fd; see the `type` table above (1 = `SOCK_STREAM` for TCP) |
| `edx` | `protocol` | **In.** Protocol number within the family and type. 0 picks the default (TCP for `AF_INET`+`SOCK_STREAM`); 6 = `IPPROTO_TCP`, 17 = `IPPROTO_UDP` |

Returns in `eax` the new socket's file descriptor: the lowest-numbered fd not currently open (often 3 in a fresh program). The socket has no address and no connection yet; pass this fd as `sockfd` to `bind`, `listen`, `connect`, etc. Negative means `-errno`. Common errors: `-EAFNOSUPPORT` (unknown family), `-EINVAL` (unknown type or flag bits), `-EPROTONOSUPPORT` (protocol doesn't fit the family/type), `-ENOSYS` (kernel < 4.3 — fall back to `socketcall`).

## Example
```nasm
    ; fd = socket(AF_INET, SOCK_STREAM, 0)  — a TCP socket
    mov eax, 359
    mov ebx, 2          ; AF_INET
    mov ecx, 1          ; SOCK_STREAM
    xor edx, edx        ; protocol 0
    int 0x80

    test eax, eax
    js .error           ; negative = -errno
    mov esi, eax        ; keep the fd somewhere safe
```

For a server, this fd is what you pass to `bind` (361), `listen` (363), and `accept4` (364).

## Linux philosophy
The return value is the whole story: a socket is just a file descriptor, so the dozens of tools built on `read`/`write`/`close`/`poll` work on the network for free. New capability, zero new I/O interface — the "everything is a file (descriptor)" idea at its best.

## In The Linux Programming Interface
Chapter 56, "Sockets: Introduction" (p.1149). The book explains the socket model (streams vs datagrams, active vs passive sockets) and walks through the full server and client call sequences you'll follow for the HTTP server.
