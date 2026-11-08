# Final Project: A Web Server in x86 Assembly

You're going to build a real HTTP server in NASM, using nothing but raw Linux
syscalls. By the end, `curl` will be able to **read data** from your server
(GET a file) and **write data** to it (POST a file). No libc, no frameworks —
just you and the kernel.

This project exercises almost everything in this repo. Before each stage, skim
the linked syscall notes in [syscalls/](syscalls/) and the TLPI chapters they
point to (especially Ch 56 *Sockets: Introduction*, Ch 59 *Internet Domains*,
and Ch 60 *Server Design*). Unfamiliar terms are in the
[glossary](glossary.md).

**The big idea** (TLPI Ch 56): a socket is just a file descriptor. Once
`accept4` hands you a connected socket, you talk HTTP with the same `read` (3)
and `write` (4) you already know. A web server is 90% file I/O.

---

## Stage 0 — Setup

You need a 32-bit-capable Linux environment. On your Mac, Docker is easiest:

```bash
docker run --rm -it -p 8080:8080 -v "$PWD":/work -w /work debian:bookworm bash
# inside the container:
apt-get update && apt-get install -y nasm binutils curl strace
```

Build/run cycle for every stage:

```bash
nasm -f elf32 server.asm && i686-linux-gnu-ld -o server server.o && qemu-i386-static ./server
./server
```

Smoke test: assemble the hello-world from
[linux-x86-32-syscalls.md](linux-x86-32-syscalls.md). If that runs, you're set.

**Checkpoint:** `./hello` prints and exits. Run `strace ./hello` and identify
the `write` and `exit` calls in the output. `strace` is your debugger for this
whole project.

---

## Stage 1 — Get a socket

Read: [syscalls/socket.md](syscalls/socket.md)

A TCP socket is created with `socket(AF_INET, SOCK_STREAM, 0)` — syscall
**359** on x86-32.

```nasm
    ; eax = socket(AF_INET=2, SOCK_STREAM=1, 0)
    mov eax, 359
    mov ebx, 2
    mov ecx, 1
    xor edx, edx
    int 0x80
    test eax, eax
    js  fail            ; negative eax = -errno
```

**Exercise:** write a program that creates a socket and exits with the fd as
its exit code (`mov ebx, eax` / `mov eax, 1` / `int 0x80`). Run it and check
`echo $?` — you should see `3` (fds 0,1,2 are taken by stdin/stdout/stderr).
That's "everything is a file descriptor" made visible.

---

## Stage 2 — Bind and listen

Read: [syscalls/bind.md](syscalls/bind.md), [syscalls/listen.md](syscalls/listen.md)

`bind` (361) attaches your socket to a port. It needs a 16-byte
`struct sockaddr_in`, which you build byte-by-byte in `.data`:

```nasm
section .data
sockaddr:
    dw 2                ; sin_family = AF_INET
    dw 0x901F           ; sin_port = 8080 in NETWORK byte order (big-endian 0x1F90)
    dd 0                ; sin_addr = 0.0.0.0 (INADDR_ANY: all interfaces)
    dd 0, 0             ; 8 bytes of zero padding
sockaddr_len equ 16
one: dd 1
```

The port is the classic trap: 8080 = `0x1F90`, but network order wants the
bytes as `1F 90`, and NASM's `dw` stores little-endian — so you write
`dw 0x901F`. If your server mysteriously listens on port 36895, you forgot
this. (TLPI §59.2 explains byte ordering.)

Also call `setsockopt` (366) with SO_REUSEADDR before binding, so restarting
your server doesn't fail with `EADDRINUSE` (-98) for a minute after each run:

```nasm
    ; setsockopt(fd, SOL_SOCKET=1, SO_REUSEADDR=2, &one, 4)
    mov eax, 366
    mov ebx, ebp        ; we keep the listening fd in ebp (see below)
    mov ecx, 1
    mov edx, 2
    mov esi, one
    mov edi, 4
    int 0x80
```

Note: `setsockopt` uses 5 arguments, which means `esi` and `edi` get used.
Keep your listening fd somewhere safe — `ebp` works nicely.

Then bind and listen:

```nasm
    ; bind(fd, &sockaddr, 16)
    mov eax, 361
    mov ebx, ebp
    mov ecx, sockaddr
    mov edx, sockaddr_len
    int 0x80
    test eax, eax
    js  fail

    ; listen(fd, 5)
    mov eax, 363
    mov ebx, ebp
    mov ecx, 5
    int 0x80
```

**Checkpoint:** your program binds, listens, then just loops forever
(`jmp $`). From another terminal: `ss -tlnp | grep 8080` shows your server
listening. Kill it with Ctrl-C.

---

## Stage 3 — Accept and speak HTTP

Read: [syscalls/accept4.md](syscalls/accept4.md), [syscalls/read.md](syscalls/read.md), [syscalls/write.md](syscalls/write.md)

`accept4` (364) blocks until a client connects, then returns a **new** fd for
that connection. You `read` the request, `write` a response, and `close`. The
listening fd stays alive for the next client.

An HTTP response is just bytes. The smallest valid one:

```
HTTP/1.0 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 20\r\n
\r\n
Hello from assembly\n
```

Here is the complete v1 server — type it in, don't paste it:

```nasm
; server_v1.asm — a web server that always says hello
; nasm -f elf32 server_v1.asm && i686-linux-gnu-ld -o server_v1 server_v1.o && qemu-i386-static ./server_v1

%define SYS_exit        1
%define SYS_read        3
%define SYS_write       4
%define SYS_close       6
%define SYS_socket      359
%define SYS_bind        361
%define SYS_listen      363
%define SYS_accept4     364
%define SYS_setsockopt  366

section .data
sockaddr:
    dw 2                ; AF_INET
    dw 0x901F           ; port 8080, network byte order
    dd 0                ; INADDR_ANY
    dd 0, 0
one: dd 1

response:
    db "HTTP/1.0 200 OK", 13, 10
    db "Content-Type: text/plain", 13, 10
    db "Content-Length: 20", 13, 10
    db 13, 10
    db "Hello from assembly", 10
response_len equ $ - response

section .bss
request resb 4096

section .text
global _start
_start:
    ; ebp = socket(AF_INET, SOCK_STREAM, 0)
    mov eax, SYS_socket
    mov ebx, 2
    mov ecx, 1
    xor edx, edx
    int 0x80
    test eax, eax
    js  fail
    mov ebp, eax

    ; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, 4)
    mov eax, SYS_setsockopt
    mov ebx, ebp
    mov ecx, 1
    mov edx, 2
    mov esi, one
    mov edi, 4
    int 0x80

    ; bind(fd, &sockaddr, 16)
    mov eax, SYS_bind
    mov ebx, ebp
    mov ecx, sockaddr
    mov edx, 16
    int 0x80
    test eax, eax
    js  fail

    ; listen(fd, 5)
    mov eax, SYS_listen
    mov ebx, ebp
    mov ecx, 5
    int 0x80

accept_loop:
    ; ebx = accept4(fd, NULL, NULL, 0)
    mov eax, SYS_accept4
    mov ebx, ebp
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    int 0x80
    test eax, eax
    js  accept_loop     ; a failed accept shouldn't kill the server
    mov ebx, eax        ; ebx = connection fd (stays put for read/write/close)

    ; read(connfd, request, 4096) — read the request (ignored for now)
    mov eax, SYS_read
    mov ecx, request
    mov edx, 4096
    int 0x80

    ; write(connfd, response, response_len)
    mov eax, SYS_write
    mov ecx, response
    mov edx, response_len
    int 0x80

    ; close(connfd)
    mov eax, SYS_close
    int 0x80

    jmp accept_loop

fail:
    mov eax, SYS_exit
    mov ebx, 1
    int 0x80
```

Notice the register discipline: the connection fd goes into `ebx` right after
`accept4` and *stays there* — `read`, `write`, and `close` all take the fd as
their first argument, so `ebx` is already correct for all three. Only `eax`
(syscall number / return value) changes between calls.

**Checkpoint:**

```bash
./server_v1 &
curl http://localhost:8080/        # -> Hello from assembly
strace -f -e trace=network,read,write ./server_v1   # watch it happen
```

Congratulations — that's a working web server. Everything from here is
refinement.

---

## Stage 4 — Actually look at the request

Read: [syscalls/read.md](syscalls/read.md) again — note that `read` returns
the byte count in `eax`.

A request from curl looks like:

```
GET /notes.txt HTTP/1.1\r\n
Host: localhost:8080\r\n
...more headers...\r\n
\r\n
```

Everything you need is in the first line: the **method** (`GET`/`POST`) and
the **path** (`/notes.txt`).

**Exercises:**

1. After `read`, `write` the raw request to stdout (fd 1) so you can see what
   curl sends. You'll need to save the connection fd elsewhere (e.g. `edi` —
   it's free between syscalls here) while `ebx` is 1 for the stdout write,
   then restore it.
2. Extract the path: it starts at `request+4` for `GET ` (`request+5` for
   `POST `) and ends at the next space. Scan for the space and overwrite it
   with a 0 byte — now you have a NUL-terminated filename to pass to `open`.
   Skip the leading `/` (serve `notes.txt`, not `/notes.txt`, or every open
   will hit the filesystem root).
3. Check the method: compare the first 4 bytes of the request against `"GET "`
   as a single 32-bit constant — `cmp dword [request], 0x20544547` (that's
   " TEG" backwards: little-endian again). For POST: `cmp dword [request],
   0x54534F50` ("TSOP").

**Hints:** a simple byte-scan loop looks like:

```nasm
    mov esi, request + 4    ; start of path
find_space:
    cmp byte [esi], ' '
    je  found
    inc esi
    jmp find_space
found:
    mov byte [esi], 0       ; NUL-terminate the path in place
```

(Real servers must bound this scan — see "Hardening" below. Fine to skip
while learning.)

---

## Stage 5 — Read data: serve real files (GET)

Read: [syscalls/open.md](syscalls/open.md), [syscalls/stat.md](syscalls/stat.md),
[syscalls/close.md](syscalls/close.md)

Plan for a GET request:

1. `open(path, O_RDONLY=0)` — syscall 5. If it returns negative, send a 404.
2. `write` the response header to the connection.
3. Loop: `read` up to 4096 bytes from the file, `write` that many bytes to
   the connection, until `read` returns 0 (EOF).
4. `close` the file, `close` the connection.

The read/write pump is the heart of it — this is `cat`, aimed at a socket:

```nasm
; edi = file fd, esi = connection fd (choose your own homes for these)
copy_loop:
    mov eax, 3              ; read(file, buf, 4096)
    mov ebx, edi
    mov ecx, filebuf
    mov edx, 4096
    int 0x80
    test eax, eax
    jle copy_done           ; 0 = EOF, negative = error

    mov edx, eax            ; write exactly as many bytes as we read
    mov eax, 4              ; write(conn, buf, n)
    mov ebx, esi
    mov ecx, filebuf
    int 0x80
    jmp copy_loop
copy_done:
```

Use two prepared responses in `.data`:

```
ok_header:  "HTTP/1.0 200 OK\r\n\r\n"
not_found:  "HTTP/1.0 404 Not Found\r\n\r\n404\n"
```

(Cheat: HTTP/1.0 with no Content-Length is legal — "response ends when the
server closes the connection." That's why we close after every request, and
it saves you computing the file size. When you want to do it properly later,
`fstat64` (197) gives you the size — offset 44 in the struct.)

**Checkpoint:**

```bash
echo "hello from a file" > notes.txt
./server_v2 &
curl http://localhost:8080/notes.txt      # -> hello from a file
curl -i http://localhost:8080/nope.txt    # -> HTTP/1.0 404 Not Found
```

---

## Stage 6 — Write data: accept uploads (POST)

Read: [syscalls/open.md](syscalls/open.md) (the O_CREAT flags),
[syscalls/write.md](syscalls/write.md)

Plan for `POST /name.txt` — the mirror image of GET:

1. Find the body: it starts right after the blank line (`\r\n\r\n`,
   bytes `0D 0A 0D 0A`). Scan the request buffer for that 4-byte pattern
   (`cmp dword [esi], 0x0A0D0A0D`). Body length = bytes read minus
   (body start - buffer start).
2. `open(path, O_WRONLY|O_CREAT|O_TRUNC = 0x241, 0o644)` — same flags as the
   file-writing example in [linux-x86-32-syscalls.md](linux-x86-32-syscalls.md).
3. `write` the body to the file, `close` it.
4. Reply `"HTTP/1.0 201 Created\r\n\r\n"`.

**Checkpoint:**

```bash
curl -d "saved by assembly" http://localhost:8080/upload.txt
curl http://localhost:8080/upload.txt     # -> saved by assembly
cat upload.txt                             # -> saved by assembly
```

When that round trip works — POST it in, GET it back — your server officially
reads and writes data. That's the project goal met.

*(Caveat you can ignore for now: this assumes the whole request arrived in one
`read`, which is true for small local requests. Real servers loop until
Content-Length bytes have arrived.)*

---

## Stage 7 — Stretch goals

- **Survive a crash mid-request:** wrap unknown methods in a
  `"HTTP/1.0 405 Method Not Allowed\r\n\r\n"` response instead of falling
  through into GET handling.
- **Concurrency** ([syscalls/fork.md](syscalls/fork.md), TLPI Ch 60): after
  `accept4`, call `fork` (2). Child: close the listening fd, handle the
  request, `exit`. Parent: close the connection fd, loop back to accept.
  This is the classic UNIX forking server — TLPI Ch 60 is exactly this design.
  (Detail: collect dead children with `waitpid` (WNOHANG=1), or ignore
  SIGCHLD, so zombie processes don't pile up — see
  [syscalls/waitpid.md](syscalls/waitpid.md) and the [glossary](glossary.md).)
- **Hardening:** bound every scan loop by the byte count `read` returned;
  reject paths containing `..` (or any `/`) so `curl http://.../../etc/passwd`
  can't escape your directory. Yes — your 200-line assembly server can have
  the same classic security hole ("path traversal") as real web servers.
  Rite of passage.
- **Content-Type by extension:** `.html` → `text/html` and everything else
  `text/plain` is enough to serve a real page to your browser.

---

## Debugging cheat sheet

| Symptom | Likely cause |
|---|---|
| `bind` returns -98 (EADDRINUSE) | old server still running, or you skipped SO_REUSEADDR |
| `bind` returns -13 (EACCES) | port < 1024 needs root — stick to 8080 |
| curl hangs forever | you never `write`, or never `close` the connection |
| curl: "empty reply from server" | server crashed mid-request — run it under `strace` |
| listens on a weird port | port bytes not swapped (`dw 0x901F`, not `dw 0x1F90`) |
| `open` fails with -2 (ENOENT) | path not NUL-terminated, or leading `/` not skipped |
| everything breaks after adding a syscall | that call clobbered a register you were relying on — map out who owns `ebx`/`esi`/`edi`/`ebp` |

`strace ./server` shows every syscall with decoded arguments — when the server
misbehaves, the answer is almost always right there. TLPI Appendix A
(p.1401) is a short guide to exactly this workflow.

## Why this is a fitting final project

The punchline of the whole exercise: once the connection is accepted, a web
server is `open`, `read`, `write`, `close` — the same four calls as a file
copier, because a socket **is** a file descriptor. Small primitives, uniform
interfaces, everything is a file. You didn't just read the Linux philosophy,
you assembled it.
