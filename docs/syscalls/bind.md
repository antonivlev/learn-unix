[← Back to index](README.md)

# bind (syscall 361)

Attach a socket to a local address and port — "I am port 8080 on this machine."

## What it does
A fresh socket has no address. `bind` gives it one, so clients know where to connect. Servers bind to a well-known port before calling `listen`. The address goes in a `struct sockaddr_in`, which you can lay out by hand in NASM — it's just 16 bytes.

## How it works
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 361 |
| `ebx` | `sockfd` | **In.** The socket to name: the fd returned by `socket`. It must not already have an address (binding twice gives `-EINVAL`) |
| `ecx` | `addr` | **In.** Address of a `struct sockaddr_in` in your memory holding the local IP and port to claim; layout below. The kernel only reads it |
| `edx` | `addrlen` | **In.** Size in bytes of the struct at `addr`: 16 for `sockaddr_in`. Smaller values give `-EINVAL` |

`struct sockaddr_in` byte layout on x86-32 (16 bytes total):

| Offset | Size | Field | Note |
|---|---|---|---|
| 0 | 2 | family | `dw 2` (AF_INET), in normal x86 (host) byte order; must match the socket's domain |
| 2 | 2 | port | **network byte order** (big-endian); 0 = kernel picks a free ephemeral port |
| 4 | 4 | IP address | network byte order; see the table below |
| 8 | 8 | padding | zeros |

The IP address field picks which local interfaces the socket accepts connections on:

| Address | NASM | Meaning | Example |
|---|---|---|---|
| `INADDR_ANY` (0.0.0.0) | `dd 0` | All interfaces: reachable from other machines and from localhost | A public server |
| `INADDR_LOOPBACK` (127.0.0.1) | `dd 0x0100007F` | Loopback only: reachable only from this machine | A dev server you don't want exposed |

Port gotcha: x86 is little-endian but the network is big-endian. Port 8080 is 0x1F90, so in NASM you write `dw 0x901F` (bytes swapped). Returns 0 in `eax` on success: the socket now owns that address and port. Negative means `-errno`. Common errors: `-EADDRINUSE` (port taken — often a just-closed server still in TCP's cooldown, see [glossary](../glossary.md): TIME_WAIT), `-EACCES` (ports < 1024 need root).

## Example
```nasm
section .data
    addr:
        dw 2            ; AF_INET
        dw 0x901F       ; port 8080 in network byte order
        dd 0            ; INADDR_ANY
        dd 0, 0         ; padding
    one: dd 1

section .text
    ; assume ebp = socket fd from socket(359)
    ; first: setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, 4)
    mov eax, 366        ; setsockopt
    mov ebx, ebp        ; fd
    mov ecx, 1          ; SOL_SOCKET
    mov edx, 2          ; SO_REUSEADDR
    mov esi, one        ; optval: pointer to int 1
    mov edi, 4          ; optlen
    int 0x80

    ; bind(fd, addr, 16)
    mov eax, 361
    mov ebx, ebp
    mov ecx, addr
    mov edx, 16
    int 0x80
```

The `setsockopt` (366) with `SO_REUSEADDR` lets you restart your server immediately instead of getting `EADDRINUSE` for a minute. Its prototype is `int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)`:

| Register | Argument | Value used | Meaning |
|---|---|---|---|
| `eax` | — | 366 | syscall number |
| `ebx` | `sockfd` | the socket fd | **In.** The socket to configure: the fd returned by `socket` |
| `ecx` | `level` | `SOL_SOCKET` = 1 | **In.** Which layer owns the option. `SOL_SOCKET` is the generic socket layer; protocol-specific options use e.g. `IPPROTO_TCP` = 6 |
| `edx` | `optname` | `SO_REUSEADDR` = 2 | **In.** Which option within that level. `SO_REUSEADDR` allows binding a port that still has connections in TIME_WAIT |
| `esi` | `optval` | pointer to int 1 | **In.** Address of the new value in your memory. For on/off options it is a 4-byte int: 1 turns the option on, 0 turns it off |
| `edi` | `optlen` | 4 | **In.** Size in bytes of the value `optval` points to: 4 for an int |

It returns 0 on success; `-ENOPROTOOPT` (-92) means the level/optname pair is unknown. Call it after `socket` and before `bind`: the option is checked at bind time.

## Linux philosophy
`bind` is deliberately dumb: it takes a pointer to bytes and a length, without caring what's inside — so the same syscall serves IPv4, IPv6, and UNIX-domain sockets. The kernel defines a tiny generic mechanism (attach address to fd) and leaves the address format to each family.

## In The Linux Programming Interface
Chapter 56, §56.3 for `bind` itself; Chapter 59, "Sockets: Internet Domains" (p.1197) for `sockaddr_in`, byte order, and why `htons` exists — the book explains what we hand-encoded above.
