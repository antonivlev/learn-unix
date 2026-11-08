# Linux System Calls — x86 (32-bit) Beginner Reference

A beginner-friendly reference for Linux system calls on **x86 32-bit** (the `int 0x80` interface), aimed at people learning C and NASM assembly.

All syscall numbers below are verified against the
[Chromium OS syscall table (x86 32-bit)](https://www.chromium.org/chromium-os/developer-library/reference/linux-constants/syscalls/#x86-32-bit).
Unfamiliar terms: see the [glossary](glossary.md).

---

## How syscalls work on x86 32-bit

You put the **syscall number in `eax`**, the arguments in registers, and execute `int 0x80`. The kernel does the work and puts the **return value in `eax`** (a negative value between -1 and -4095 means an error: it's `-errno`).

| Register | Purpose |
|---|---|
| `eax` | syscall number (and return value) |
| `ebx` | 1st argument |
| `ecx` | 2nd argument |
| `edx` | 3rd argument |
| `esi` | 4th argument |
| `edi` | 5th argument |
| `ebp` | 6th argument |

In C you normally call the libc wrappers (`write()`, `open()`, ...), but you can invoke any syscall directly with `syscall(SYS_xxx, ...)` from `<sys/syscall.h>`.

> **Tip:** run any program under `strace ./prog` to watch every syscall it makes. It's the best learning tool for this topic.

---

## Most commonly used syscalls

These are the ones you'll actually use while learning. Learn `read`, `write`, `open`, `close`, and `exit` first — you can write real programs with just those five.

| # (`eax`) | Name | Arguments (`ebx`, `ecx`, `edx`, ...) | Description |
|---|---|---|---|
| 1 | `exit` | exit_code | Terminate the calling thread with an exit status (0 = success). |
| 2 | `fork` | — | Duplicate the current process. Returns child PID to the parent, 0 to the child. |
| 3 | `read` | fd, buffer, count | Read up to `count` bytes from file descriptor `fd` into `buffer`. Returns bytes read (0 = end of file). fd 0 is stdin. |
| 4 | `write` | fd, buffer, count | Write `count` bytes from `buffer` to `fd`. Returns bytes written. fd 1 is stdout, fd 2 is stderr. |
| 5 | `open` | pathname, flags, mode | Open a file, returns a file descriptor. Flags: `O_RDONLY`=0, `O_WRONLY`=1, `O_RDWR`=2, `O_CREAT`=0x40, `O_TRUNC`=0x200, `O_APPEND`=0x400. |
| 6 | `close` | fd | Close a file descriptor. |
| 7 | `waitpid` | pid, status_ptr, options | Wait for a child process to change state (typically: to exit). |
| 8 | `creat` | pathname, mode | Create a file (same as `open` with `O_CREAT\|O_WRONLY\|O_TRUNC`). |
| 10 | `unlink` | pathname | Delete a file (remove a directory entry). |
| 11 | `execve` | pathname, argv, envp | Replace the current process with a new program. On success it never returns. |
| 12 | `chdir` | pathname | Change the current working directory. |
| 13 | `time` | time_t_ptr (or NULL) | Get current time as seconds since the Unix epoch. Returned in `eax`. |
| 19 | `lseek` | fd, offset, whence | Move the file read/write position. `whence`: 0=`SEEK_SET`, 1=`SEEK_CUR`, 2=`SEEK_END`. |
| 20 | `getpid` | — | Return the process ID of the caller. |
| 33 | `access` | pathname, mode | Check if a file exists / is readable / writable / executable (`F_OK`=0, `X_OK`=1, `W_OK`=2, `R_OK`=4). |
| 37 | `kill` | pid, signal | Send a signal to a process (e.g. `SIGTERM`=15, `SIGKILL`=9). |
| 38 | `rename` | oldpath, newpath | Rename or move a file. |
| 39 | `mkdir` | pathname, mode | Create a directory (mode e.g. `0755` = octal `0o755`). |
| 40 | `rmdir` | pathname | Remove an empty directory. |
| 41 | `dup` | fd | Duplicate a file descriptor (returns the lowest free fd). |
| 42 | `pipe` | int_array[2] | Create a pipe: `array[0]` = read end, `array[1]` = write end. |
| 45 | `brk` | new_break | Grow/shrink the heap (this is what `malloc` uses under the hood). Call with 0 to query the current break. |
| 54 | `ioctl` | fd, request, arg | Device-specific control operations (e.g. terminal settings, window size). |
| 63 | `dup2` | oldfd, newfd | Duplicate `oldfd` onto a specific number `newfd` (used for I/O redirection). |
| 78 | `gettimeofday` | timeval_ptr, tz_ptr | Get wall-clock time with microsecond precision. |
| 85 | `readlink` | pathname, buffer, bufsize | Read the target of a symbolic link. |
| 90 | `mmap` | args_struct_ptr | Map files or anonymous memory into the address space (old interface; prefer `mmap2`). |
| 91 | `munmap` | addr, length | Unmap a memory region created by `mmap`/`mmap2`. |
| 102 | `socketcall` | call_number, args_ptr | The classic 32-bit single entry point for **all** socket operations (socket=1, bind=2, connect=3, listen=4, accept=5, send=9, recv=10...). Modern kernels (≥ 4.3) also have direct syscalls, see 359+ below. |
| 106 | `stat` | pathname, stat_buf | Get file metadata (size, permissions, timestamps). On modern 32-bit code prefer `stat64` (195). |
| 108 | `fstat` | fd, stat_buf | Like `stat`, but for an already-open file descriptor (prefer `fstat64`, 197). |
| 114 | `wait4` | pid, status_ptr, options, rusage | Wait for a child and optionally get resource usage (what libc `waitpid` actually calls). |
| 118 | `fsync` | fd | Flush a file's data and metadata to disk. |
| 122 | `uname` | utsname_buf | Get kernel name, version, and architecture info. |
| 125 | `mprotect` | addr, length, prot | Change memory protection (read/write/exec) of a region. |
| 141 | `getdents` | fd, dirent_buf, count | Read directory entries (how `ls` works; prefer `getdents64`, 220). |
| 146 | `writev` | fd, iovec_array, count | Write from multiple buffers in one call ("scatter/gather" I/O). |
| 162 | `nanosleep` | timespec_req, timespec_rem | Sleep with nanosecond precision. |
| 168 | `poll` | pollfd_array, nfds, timeout | Wait for events (readable/writable) on multiple file descriptors. |
| 174 | `rt_sigaction` | signum, act, oldact, sigsetsize | Install a signal handler (what libc `signal()`/`sigaction()` use). |
| 183 | `getcwd` | buffer, size | Get the current working directory. |
| 192 | `mmap2` | addr, length, prot, flags, fd, pgoffset | The modern 32-bit `mmap`: allocate memory or map files. For anonymous memory: `prot=3` (RW), `flags=0x22` (`MAP_PRIVATE\|MAP_ANONYMOUS`), `fd=-1`. |
| 195 | `stat64` | pathname, stat64_buf | 64-bit-capable `stat` — use this instead of 106 on 32-bit. |
| 197 | `fstat64` | fd, stat64_buf | 64-bit-capable `fstat`. |
| 220 | `getdents64` | fd, dirent64_buf, count | Modern directory listing. |
| 240 | `futex` | uaddr, op, val, ... | Fast userspace locking — the primitive behind mutexes and condition variables. |
| 252 | `exit_group` | exit_code | Exit **all** threads in the process. This is what C's `exit()` actually calls. Prefer this over `exit` (1) in multi-threaded programs. |
| 265 | `clock_gettime` | clockid, timespec_buf | High-resolution time (`CLOCK_REALTIME`=0, `CLOCK_MONOTONIC`=1). |
| 295 | `openat` | dirfd, pathname, flags, mode | Modern `open` relative to a directory fd. Use `AT_FDCWD` (-100) as `dirfd` to behave like plain `open`. |
| 331 | `pipe2` | int_array[2], flags | `pipe` with flags (e.g. `O_CLOEXEC`, `O_NONBLOCK`). |
| 355 | `getrandom` | buffer, count, flags | Fill a buffer with cryptographically secure random bytes. |
| 359 | `socket` | domain, type, protocol | Create a socket directly (kernel ≥ 4.3). `AF_INET`=2, `SOCK_STREAM`=1. |
| 361 | `bind` | sockfd, sockaddr, addrlen | Bind a socket to an address/port. |
| 362 | `connect` | sockfd, sockaddr, addrlen | Connect a socket to a remote address. |
| 363 | `listen` | sockfd, backlog | Mark a socket as accepting connections. |
| 364 | `accept4` | sockfd, addr, addrlen, flags | Accept an incoming connection. |

---

## Less commonly used syscalls

You'll rarely need these while learning, but you'll see them in `strace` output and older tutorials.

| # (`eax`) | Name | Description |
|---|---|---|
| 0 | `restart_syscall` | Kernel-internal: restarts an interrupted syscall. Never called directly. |
| 9 | `link` | Create a hard link to a file. |
| 14 | `mknod` | Create a device file or FIFO. |
| 15 | `chmod` | Change file permissions. |
| 21 | `mount` | Mount a filesystem (requires root). |
| 23 | `setuid` | Set user identity (16-bit legacy; `setuid32` is 213). |
| 24 | `getuid` | Get real user ID (16-bit legacy; use `getuid32`, 199). |
| 26 | `ptrace` | Trace/debug another process — how `gdb` and `strace` work. |
| 27 | `alarm` | Schedule a `SIGALRM` signal after N seconds. |
| 29 | `pause` | Sleep until a signal arrives. |
| 30 | `utime` | Change a file's access/modification timestamps. |
| 36 | `sync` | Flush all filesystem buffers to disk. |
| 43 | `times` | Get process CPU times. |
| 48 | `signal` | Ancient signal-handler API — use `rt_sigaction` (174) instead. |
| 52 | `umount2` | Unmount a filesystem. |
| 55 | `fcntl` | Misc. file descriptor control (locks, flags); `fcntl64` is 221. |
| 60 | `umask` | Set the default permission mask for new files. |
| 64 | `getppid` | Get parent process ID. |
| 65 | `getpgrp` | Get process group ID. |
| 66 | `setsid` | Create a new session (used by daemons). |
| 67 | `sigaction` | Old signal API — superseded by `rt_sigaction` (174). |
| 82 | `select` | Old way to wait on multiple fds at once (via a struct; `_newselect` is 142) — prefer `poll` (168). |
| 83 | `symlink` | Create a symbolic link. |
| 88 | `reboot` | Reboot the machine (requires root + magic numbers). |
| 92 | `truncate` | Cut a file to a given length by path (`ftruncate` = 93, by fd). |
| 116 | `sysinfo` | Get system stats: uptime, free RAM, load average. |
| 119 | `sigreturn` | Kernel-internal: return from a signal handler (`rt_sigreturn` = 173). |
| 120 | `clone` | Low-level `fork` with fine-grained sharing — how threads are created. |
| 132 | `getpgid` | Get process group of an arbitrary process. |
| 140 | `_llseek` | 64-bit file seeking on 32-bit systems (what libc `lseek64` uses). |
| 145 | `readv` | Read into multiple buffers in one call. |
| 158 | `sched_yield` | Give up the CPU so another thread can run. |
| 173 | `rt_sigreturn` | Kernel-internal: return from a signal handler. |
| 180 | `pread64` | Read from a file at a given offset without moving the file position. |
| 181 | `pwrite64` | Write to a file at a given offset without moving the file position. |
| 182 | `chown` | Change file owner (16-bit legacy; `chown32` is 212). |
| 190 | `vfork` | Like `fork` but shares memory until `execve` — easy to misuse, avoid. |
| 196 | `lstat64` | Like `stat64` but doesn't follow symlinks. |
| 199 | `getuid32` | Get real user ID (modern 32-bit version). |
| 201 | `geteuid32` | Get effective user ID. |
| 224 | `gettid` | Get the calling **thread's** ID. |
| 267 | `clock_nanosleep` | Sleep against a specific clock (e.g. absolute deadline). |
| 301 | `unlinkat` | Delete a file relative to a directory fd. |
| 330 | `dup3` | `dup2` with flags. |
| 369 | `sendto` | Send data on a socket (direct syscall, kernel ≥ 4.3). |
| 371 | `recvfrom` | Receive data on a socket (direct syscall, kernel ≥ 4.3). |
| 383 | `statx` | Newest, most complete file-metadata syscall. |
| 403 | `clock_gettime64` | Year-2038-safe `clock_gettime` for 32-bit systems. |

> Note: on 32-bit x86 there was no `send`/`recv` direct syscall — use `socketcall` (102) or `sendto`/`recvfrom` with a NULL address.

---

## Examples

### 1. Hello world (NASM, pure syscalls)

```nasm
; nasm -f elf32 hello.asm && i686-linux-gnu-ld -o hello hello.o && qemu-i386-static ./hello
section .data
    msg     db  "Hello, world!", 10   ; 10 = '\n'
    msglen  equ $ - msg

section .text
global _start
_start:
    ; write(1, msg, msglen)
    mov eax, 4          ; syscall number: write
    mov ebx, 1          ; fd 1 = stdout
    mov ecx, msg        ; buffer
    mov edx, msglen     ; count
    int 0x80

    ; exit(0)
    mov eax, 1          ; syscall number: exit
    xor ebx, ebx        ; exit code 0
    int 0x80
```

### 2. Read from stdin, echo back (NASM)

```nasm
; nasm -f elf32 echo.asm && i686-linux-gnu-ld -o echo echo.o && qemu-i386-static ./echo
section .bss
    buf resb 128

section .text
global _start
_start:
    ; eax = read(0, buf, 128)
    mov eax, 3          ; read
    mov ebx, 0          ; fd 0 = stdin
    mov ecx, buf
    mov edx, 128
    int 0x80

    ; write(1, buf, bytes_read)
    mov edx, eax        ; count = return value of read
    mov eax, 4          ; write
    mov ebx, 1          ; stdout
    mov ecx, buf
    int 0x80

    mov eax, 1          ; exit
    xor ebx, ebx
    int 0x80
```

### 3. Create and write a file (NASM)

```nasm
; nasm -f elf32 mkfile.asm && i686-linux-gnu-ld -o mkfile mkfile.o && qemu-i386-static ./mkfile
section .data
    fname   db "out.txt", 0            ; C strings are NUL-terminated!
    text    db "written by asm", 10
    textlen equ $ - text

section .text
global _start
_start:
    ; fd = open("out.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov eax, 5          ; open
    mov ebx, fname
    mov ecx, 0x241      ; O_WRONLY(1) | O_CREAT(0x40) | O_TRUNC(0x200)
    mov edx, 0o644      ; rw-r--r--
    int 0x80

    ; error check: negative eax means -errno
    test eax, eax
    js .exit_fail

    mov ebx, eax        ; save fd for write & close

    ; write(fd, text, textlen)
    mov eax, 4
    mov ecx, text
    mov edx, textlen
    int 0x80

    ; close(fd)  -- ebx still holds fd
    mov eax, 6
    int 0x80

    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80

.exit_fail:
    mov eax, 1
    mov ebx, 1          ; exit(1)
    int 0x80
```

### 4. fork + execve (NASM sketch)

```nasm
    ; pid = fork()
    mov eax, 2
    int 0x80
    test eax, eax
    jnz .parent         ; parent gets child PID, child gets 0

    ; child: execve("/bin/ls", argv, NULL)
    mov eax, 11
    mov ebx, path       ; db "/bin/ls", 0
    mov ecx, argv       ; dd path, 0   (array of char*, NULL-terminated)
    xor edx, edx        ; envp = NULL
    int 0x80
    ; only reached if execve failed

.parent:
    ; waitpid(pid, NULL, 0)
    mov ebx, eax
    mov eax, 7
    xor ecx, ecx
    xor edx, edx
    int 0x80
```

### 5. Direct syscalls from C

```c
/* gcc -m32 -o demo demo.c */
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
    /* Same as write(1, ...) but bypassing the libc wrapper */
    syscall(SYS_write, 1, "hi from raw syscall\n", 20);

    /* getpid via raw syscall */
    long pid = syscall(SYS_getpid);
    (void)pid;

    return 0;  /* libc turns this into exit_group(0) */
}
```

### 6. Inline `int 0x80` from C (for the curious)

```c
/* gcc -m32 -o inline inline.c */
int main(void) {
    const char msg[] = "inline asm!\n";
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)                       /* eax -> ret */
        : "a"(4), "b"(1), "c"(msg), "d"(sizeof(msg) - 1)  /* write(1, msg, len) */
        : "memory"
    );
    return 0;
}
```

---

## Quick tips

- **Error handling:** the kernel returns `-errno` in `eax`. If `eax` is between -4095 and -1, it's an error. E.g. -2 means `ENOENT` (no such file). Check `errno -l` in your shell or `man 3 errno`.
- **Strings must be NUL-terminated** when passed as paths — assembly won't do this for you (`db "file.txt", 0`).
- **Watch real programs:** `strace -f ./prog` shows every syscall with decoded arguments.
- **Look up any syscall:** `man 2 write`, `man 2 open`, etc. Section 2 of the manual is exactly the syscall API.
- **Building 32-bit on a 64-bit machine:** `nasm -f elf32 file.asm && i686-linux-gnu-ld -o prog file.o`, then run with `qemu-i386-static ./prog` (in the dev container; on a real x86-64 Linux, plain `ld -m elf_i386` and `./prog` work).
- **Modern note:** 64-bit Linux uses completely different syscall numbers and the `syscall` instruction with different registers (`rdi`, `rsi`, `rdx`, ...). Don't mix the two tables!
