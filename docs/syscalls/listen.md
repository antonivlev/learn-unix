[← Back to index](README.md)

# listen (syscall 363)

Flip a bound socket into server mode so incoming connections start queueing up.

## What it does
After `bind`, the socket has an address but still refuses connections. `listen` marks it *passive*: the kernel now completes TCP handshakes on your behalf and parks finished connections in a queue until you collect them with `accept4`. The `backlog` argument caps that queue of completed connections waiting for `accept4`. When the queue is full, Linux drops new TCP handshake attempts and the client's kernel retries them, so clients see a delay.

| `backlog` | Effect | Example |
|---|---|---|
| 1 … `somaxconn` | Queue holds about that many finished connections | `listen(fd, 16)` for a small server |
| above `somaxconn`, or negative | Silently capped to `/proc/sys/net/core/somaxconn` (4096 by default since Linux 5.4, 128 before) | `listen(fd, 4096)` for "as many as the system allows" |
| 0 | Smallest queue Linux allows (still lets one connection wait) | `listen(fd, 0)` |

## How it works
```c
int listen(int sockfd, int backlog);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 363 |
| `ebx` | `sockfd` | **In.** The socket to turn into a server: the fd from `socket` (a `SOCK_STREAM` socket), normally already given a port with `bind`. If it is unbound, the kernel binds it to a random free port |
| `ecx` | `backlog` | **In.** Maximum number of finished connections that may wait in the queue for `accept4`, counted in connections (e.g. 16). See the `backlog` table above for 0, negative and oversized values |

Returns 0 in `eax` on success: the socket is now a listening socket, and you pass it to `accept4` from here on. Negative means `-errno`. Common errors: `-EOPNOTSUPP` (socket type can't listen, e.g. `SOCK_DGRAM`), `-EADDRINUSE` (another socket is already listening on the same address and port).

## Example
```nasm
    ; assume esi = fd from socket(359), already bound via bind(361)
    ; listen(fd, 16)
    mov eax, 363
    mov ebx, esi
    mov ecx, 16         ; backlog
    int 0x80
    test eax, eax
    js .error
    ; the kernel now accepts TCP handshakes for us;
    ; next step: accept4 (364) in a loop
```

The full server sequence: `socket` (359) → `bind` (361) → `listen` (363) → `accept4` (364).

## Linux philosophy
`listen` splits the work cleanly: the kernel handles the concurrent, timing-sensitive part (handshakes arriving whenever they like) while your program consumes ready connections one `accept4` at a time, at its own pace. The backlog queue is the buffer that decouples the two — the same producer/consumer idea as a pipe.

## In The Linux Programming Interface
Chapter 56, §56.5.1 covers `listen` and the passive-socket model; Chapter 61, §61.1 digs into what backlog really limits on Linux (completed vs pending connections) and how `somaxconn` caps it.
