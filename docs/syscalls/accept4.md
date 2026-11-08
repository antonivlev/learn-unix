[← Back to index](README.md)

# accept4 (syscall 364)

Take the next finished connection off a listening socket's queue and get a brand-new fd for it.

## What it does
`accept4` dequeues one connection that `listen` has prepared. Crucially, it returns a **new** file descriptor for that one client — the listening socket is untouched and keeps listening. If the queue is empty and the listening socket is blocking (the default), `accept4` waits until someone connects; if the listening socket was made non-blocking, it returns `-EAGAIN` instead. A server is essentially a loop: accept, handle, close, repeat.

`accept4` is `accept` plus a `flags` argument, which sets options on the **new** connection fd (the listening socket is unaffected). Combine them with OR:

| `flags` | Value | Meaning | Example |
|---|---|---|---|
| 0 | 0 | Plain blocking fd, inherited across `execve` (same as old `accept`) | `accept4(lfd, NULL, NULL, 0)` |
| `SOCK_NONBLOCK` | 0x800 | `read`/`write` on the new fd return `-EAGAIN` when they would otherwise wait; for `poll`-based servers | `accept4(lfd, NULL, NULL, SOCK_NONBLOCK)` |
| `SOCK_CLOEXEC` | 0x80000 | The new fd closes automatically on `execve` | `accept4(lfd, NULL, NULL, SOCK_NONBLOCK\|SOCK_CLOEXEC)` = flags 0x80800 |

`addr` and `addrlen` select whether you learn who connected:

| `addr`, `addrlen` | Effect |
|---|---|
| both NULL (0) | The client's address is discarded |
| pointer to a buffer, pointer to an int holding the buffer size (16 for `sockaddr_in`) | The kernel writes the client's address (family, port, IP) into the buffer and the actual length into the int |

## How it works
```c
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 364 |
| `ebx` | `sockfd` | **In.** The listening socket: the fd from `socket`, after `bind` and `listen`. It stays open and keeps listening |
| `ecx` | `addr` | **Out.** Address of a buffer in your memory where the kernel writes the client's address as a `struct sockaddr_in` (layout below); 16 bytes is enough for IPv4. 0 (NULL) = don't tell me who connected |
| `edx` | `addrlen` | **In/out.** Address of a 4-byte int in your memory. Going in, it must hold the size of the `addr` buffer in bytes (16). Coming out, the kernel overwrites it with the actual size of the client's address (16 for IPv4). If your buffer was smaller, the address is cut short and the int holds the full size. Ignored when `addr` is NULL; pass 0 then too |
| `esi` | `flags` | **In.** Options for the new connection fd, ORed together; 0 = none. See the `flags` table above |

What the kernel writes into `addr` for an IPv4 client (`struct sockaddr_in`, 16 bytes):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 2 | family | 2 (`AF_INET`), host byte order |
| 2 | 2 | port | Client's port, network byte order (big-endian): swap the bytes to read it on x86 |
| 4 | 4 | addr | Client's IPv4 address, network byte order: the first byte in memory is the first number of the dotted address |
| 8 | 8 | zero | Padding, written as zeros |

Passing NULL for `addr`/`addrlen` is perfectly fine for a simple server. Returns in `eax` the new connected fd: a socket for talking to this one client with `read`/`write`, which you `close` when done. Negative means `-errno`. Common errors: `-EINTR` (interrupted by a signal — retry), `-EAGAIN` (non-blocking listening socket and no connection waiting), `-EMFILE` (out of fds; you forgot to close connections), `-EINVAL` (socket isn't listening, or unknown flag bits).

## Example
```nasm
    ; assume edi = listening fd (socket+bind+listen done)
    ; note: flags is arg 4 = esi, so DON'T park the listening fd there
.accept_loop:
    ; conn = accept4(listenfd, NULL, NULL, 0)
    mov eax, 364
    mov ebx, edi        ; listening fd
    xor ecx, ecx        ; addr = NULL
    xor edx, edx        ; addrlen = NULL
    xor esi, esi        ; flags = 0
    int 0x80
    test eax, eax
    js .accept_loop     ; error (e.g. -EINTR): try again
    mov ebp, eax        ; ebp = connection fd

    ; write(conn, response, resplen) — serve the client
    mov eax, 4
    mov ebx, ebp
    mov ecx, response
    mov edx, resplen
    int 0x80

    ; close(conn) — but the listening socket stays open
    mov eax, 6
    mov ebx, ebp
    int 0x80
    jmp .accept_loop
```

## Linux philosophy
One socket per role: the listening fd represents "the service", each accepted fd represents "one conversation". Because each connection is its own fd, the rest of the server is plain `read`/`write`/`close` — and you can hand the fd to a forked child or a `poll` loop without any special socket knowledge.

## In The Linux Programming Interface
Chapter 56, §56.5.2 for `accept`; Chapter 60, "Sockets: Server Design" (p.1239) shows how to build iterative and concurrent (fork-per-client) servers around exactly this accept loop — the blueprint for your HTTP server.
