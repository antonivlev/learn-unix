[← Back to index](README.md)

# poll (syscall 168)

Wait on many file descriptors at once and be told which ones are ready to read or write.

## What it does
A `read` on a socket with no data blocks your whole program. A server talking to many clients can't afford that: it needs to sleep until *any* client has data. `poll` takes an array of fds, each tagged with the events you care about, blocks until something happens (or a timeout), and marks each entry with what actually occurred. It's the simple heart of every event loop (`epoll` is the scalable successor).

## How it works
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
struct pollfd { int fd; short events; short revents; };  /* 8 bytes on x86-32 */
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 168 |
| `ebx` | `fds` | **In/out.** Address of an array of `nfds` `struct pollfd` entries in your memory (8 bytes each, layout below). You fill in `fd` and `events`; the kernel writes `revents` in every entry |
| `ecx` | `nfds` | **In.** Number of entries in the `fds` array, counted in entries (the array is `nfds` × 8 bytes). 0 is allowed and makes `poll` just sleep for `timeout` |
| `edx` | `timeout` | **In.** How long to wait, in milliseconds; -1 = wait forever, 0 = check and return at once. See the `timeout` table below |

`struct pollfd` byte layout (8 bytes; entry *i* starts at `fds + 8*i`):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `fd` | **In.** File descriptor to watch (from `socket`, `accept4`, `open`, 0 for stdin, …); negative = skip this entry |
| 4 | 2 | `events` | **In.** Bits you want to wait for, ORed together (e.g. 1 = `POLLIN`); see the bit table below |
| 6 | 2 | `revents` | **Out.** Bits that are actually true now, written by the kernel; 0 = nothing happened on this fd |

The `timeout` argument selects how long `poll` may block:

| `timeout` | Behaviour | Example |
|---|---|---|
| -1 (any negative) | Block until at least one fd is ready or a signal arrives | `poll(fds, n, -1)` in a server's main loop |
| 0 | Check once and return immediately | `poll(fds, n, 0)` to test readiness without waiting |
| > 0 | Block up to that many milliseconds, then return 0 if nothing is ready | `poll(fds, 1, 5000)` waits up to 5 s |

You set `events` per fd as an OR of the bits you want; the kernel fills `revents` with what actually happened. `POLLERR`, `POLLHUP`, and `POLLNVAL` are always reported in `revents`, whether or not you asked for them:

| Bit | Value | In `revents` it means | Example |
|---|---|---|---|
| `POLLIN` | 1 | Data to read, EOF, or (on a listening socket) a connection ready for `accept4` | `events = POLLIN` on a client or listening socket |
| `POLLPRI` | 2 | Urgent/exceptional data (TCP out-of-band) | rarely needed |
| `POLLOUT` | 4 | Writing won't block (buffer space free; or a non-blocking `connect` finished) | `events = POLLOUT` while you have pending output |
| `POLLERR` | 8 | Error on the fd (e.g. a failed connection) | output only |
| `POLLHUP` | 0x10 | Hang-up: the other end closed (e.g. all pipe writers gone) | output only |
| `POLLNVAL` | 0x20 | `fd` isn't an open file descriptor | output only |

An entry with a negative `fd` is skipped and gets `revents` = 0, which is a handy way to disable a slot without reshuffling the array. Returns in `eax` the number of entries with nonzero `revents` (0 = the timeout ran out and nothing is ready); scan the array to find which ones. Negative means `-errno`. Common errors: `-EINTR` (interrupted by a signal — just retry), `-EFAULT` (bad pointer).

## Example
```nasm
section .bss
    pfd resb 8              ; one struct pollfd

section .text
    ; wait up to 5s for stdin to become readable
    mov dword [pfd], 0      ; fd = 0 (stdin)
    mov word [pfd + 4], 1   ; events = POLLIN
    mov word [pfd + 6], 0   ; revents = 0

    mov eax, 168            ; poll(pfd, 1, 5000)
    mov ebx, pfd
    mov ecx, 1
    mov edx, 5000
    int 0x80

    test eax, eax
    jz .timeout             ; eax = 0 -> nothing ready
    test word [pfd + 6], 1  ; POLLIN set in revents?
    jnz .readable
```

## Linux philosophy
Because sockets, pipes, and terminals are all file descriptors, one syscall can wait on all of them uniformly — `poll` doesn't know or care what's behind each fd. That uniformity is what makes a single-threaded server handling a keyboard, a listening socket, and fifty clients a natural design on UNIX.

## In The Linux Programming Interface
Chapter 63, "Alternative I/O Models" (p.1325), §63.2. The book compares `select`, `poll`, and `epoll`, explains level-triggered readiness, and shows why readiness-based loops beat one-thread-per-client.
