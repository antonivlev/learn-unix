# Glossary

Plain definitions of the systems terms used in these notes.

**atomic** — happens completely or not at all; never observable half-done. `rename` is atomic: readers see the old file or the new one, never a mix.

**backlog** — the kernel's queue of established connections your program hasn't `accept`ed yet. The `listen` argument caps its length.

**blocking call** — a syscall that sleeps until it has something to return. `read` on an empty pipe blocks; `accept4` blocks until a client connects.

**copy-on-write** — after `fork`, parent and child share memory; a page is copied only when one of them writes to it. Makes fork cheap.

**EOF (end of file)** — "no more data". `read` returns 0. Not an error.

**errno** — Linux's error codes (`ENOENT`=2 no such file, `EACCES`=13 permission denied). C sets the `errno` variable; raw syscalls return the code negated in `eax` (-2 = ENOENT). See them all: `errno -l`.

**file descriptor (fd)** — a small integer handle to something open: file, socket, pipe, terminal. 0/1/2 = stdin/stdout/stderr; each `open` returns the lowest free number.

**guard page** — memory made deliberately inaccessible so an overrun crashes immediately instead of silently corrupting data.

**hard link** — a directory entry pointing at an inode. A file can have several names; all are equally "the file".

**inode** — the filesystem's record for a file: size, permissions, owner, timestamps, location on disk. Names are separate: a directory just maps names to inodes.

**IPC (inter-process communication)** — any way processes exchange data: pipes, sockets, shared memory.

**link count** — how many names point at an inode. Data is freed when it hits zero and no process has the file open.

**mechanism, not policy** — UNIX design habit: the kernel provides the capability, programs decide how to use it. `read` hands you bytes and imposes no structure.

**network byte order** — network protocols send numbers big-endian; x86 is little-endian. Ports and addresses must be byte-swapped (C's `htons()`).

**NUL-terminated string** — C convention: text ends with a zero byte. Every path passed to the kernel needs one — in NASM, `db "file.txt", 0`.

**page / page-aligned** — memory is managed in 4096-byte pages. Page-aligned = a multiple of 4096. `mprotect` works on whole pages only.

**race condition** — a bug where the outcome depends on timing between processes. Common shape: the **check-then-use race** — check a file (`access`), act on it (`open`), but it changed in the gap. Security papers call this TOCTOU.

**reap** — collect a dead child's exit status with `waitpid`, letting the kernel forget the process.

**scatter-gather I/O** — one call, several buffers: `writev` gathers pieces to write; `readv` scatters incoming data.

**set-UID program** — runs with its *owner's* permissions, not the launcher's (e.g. `passwd` runs as root). Why processes have both a "real" and an "effective" user ID.

**signal** — a software interruption sent to a process: Ctrl-C = SIGINT (2), `kill` default = SIGTERM (15), bad memory access = SIGSEGV (11). A process can handle, ignore, or accept the default (often: die).

**socket** — an fd for a network endpoint. Once connected, `read`/`write`/`close` work on it like on a file.

**TIME_WAIT** — TCP's ~1-minute cooldown on a just-closed connection's address. Why a restarted server hits "address already in use" without SO_REUSEADDR.

**user space / kernel space** — your program runs with limited powers; the kernel runs with full control. A syscall is the doorway between them.

**write-or-execute rule (W^X)** — a memory page may be writable or executable, never both. Stops attacker data from being run as code.

**zombie process** — finished, but exit status not yet collected by the parent. The kernel keeps a stub (`<defunct>` in `ps`) until it's reaped.
