[← Back to index](README.md)

# connect (syscall 362)

The client side of TCP: reach out to a server's address and port and establish a connection.

## What it does
Where a server does `bind` + `listen` + `accept4`, a client does just one thing: `connect`. You give it a socket and the server's address (same 16-byte `sockaddr_in` as `bind` uses, with family `AF_INET`=2 and the server's port and IP in network byte order). What happens next depends on the socket's type and blocking mode:

| Socket | What `connect` does | Example |
|---|---|---|
| `SOCK_STREAM` (blocking, the default) | Performs the TCP handshake and returns 0 once the connection is up | `connect(fd, &addr, 16)` after `socket(AF_INET, SOCK_STREAM, 0)` |
| `SOCK_STREAM` + `SOCK_NONBLOCK` | Starts the handshake and returns `-EINPROGRESS` at once; `poll` for `POLLOUT` to learn when it finishes | `connect(fd, &addr, 16)` after `socket(AF_INET, SOCK_STREAM\|SOCK_NONBLOCK, 0)` |
| `SOCK_DGRAM` | Sends nothing; only records a default destination so `write` works and datagrams from other addresses are filtered out | `connect(udpfd, &addr, 16)` |

When it returns successfully on a stream socket, the fd is a live two-way byte stream — `write` sends to the server, `read` receives.

## How it works
```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 362 |
| `ebx` | `sockfd` | **In.** Your client socket: the fd returned by `socket`. No `bind` needed; the kernel picks a free local port for you |
| `ecx` | `addr` | **In.** Address of a `struct sockaddr_in` in your memory holding the server's IP and port (layout below). The kernel only reads it |
| `edx` | `addrlen` | **In.** Size in bytes of the struct at `addr`: 16 for `sockaddr_in`. Smaller values give `-EINVAL` |

`struct sockaddr_in` byte layout (16 bytes total):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 2 | family | `dw 2` (`AF_INET`), host byte order |
| 2 | 2 | port | Server's port, network byte order (big-endian): port 8080 = 0x1F90 is written `dw 0x901F` |
| 4 | 4 | addr | Server's IPv4 address, network byte order: 127.0.0.1 is written `dd 0x0100007F` |
| 8 | 8 | zero | Padding; fill with zeros |

Returns 0 in `eax` on success: for a stream socket the connection is established and the same fd is ready for `read`/`write`. Negative means `-errno`. Common errors: `-ECONNREFUSED` (nothing listening on that port), `-ETIMEDOUT` (host unreachable / dropping packets), `-EINPROGRESS` (non-blocking socket; the handshake continues in the background).

## Example
```nasm
section .data
    addr:
        dw 2            ; AF_INET
        dw 0x901F       ; port 8080, network byte order
        dd 0x0100007F   ; 127.0.0.1 (bytes 7F 00 00 01 in memory)
        dd 0, 0         ; padding

section .text
    ; fd = socket(AF_INET, SOCK_STREAM, 0)
    mov eax, 359
    mov ebx, 2
    mov ecx, 1
    xor edx, edx
    int 0x80
    mov esi, eax        ; save fd

    ; connect(fd, addr, 16)
    mov eax, 362
    mov ebx, esi
    mov ecx, addr
    mov edx, 16
    int 0x80
    test eax, eax
    js .error
    ; now write(esi, ...) / read(esi, ...) talk to the server
```

Note the IP trick: 127.0.0.1 is the bytes `7F 00 00 01` on the wire, which on little-endian x86 you write as `dd 0x0100007F`.

## Linux philosophy
After `connect` succeeds there is nothing socket-specific left to learn: the fd behaves like an open file, and a client is just `connect` followed by ordinary `write`/`read`/`close`. This is a handy way to test your HTTP server without a browser.

## In The Linux Programming Interface
Chapter 56, §56.4. The book shows the full stream-socket client lifecycle and pairs it with a matching server example, then Chapter 59 covers real internet addresses and DNS lookup.
