# Syscall notes — index

One brief file per common Linux x86 32-bit syscall: what it does, how to call
it from NASM (`int 0x80`), an example, the UNIX-philosophy angle, and where to
read more in *The Linux Programming Interface* (TLPI).

Numbers reference: [../linux-x86-32-syscalls.md](../linux-x86-32-syscalls.md).
Unfamiliar terms: [../glossary.md](../glossary.md).
Capstone: [../final_project.md](../final_project.md) — a web server in assembly.

## File I/O — TLPI Ch 4, 5, 13

| File | Syscalls | TLPI |
|---|---|---|
| [open.md](open.md) | open (5), creat (8), openat (295) | Ch 4 §4.3 |
| [read.md](read.md) | read (3) | Ch 4 §4.4 |
| [write.md](write.md) | write (4) | Ch 4 §4.5 |
| [close.md](close.md) | close (6) | Ch 4 §4.6 |
| [lseek.md](lseek.md) | lseek (19), _llseek (140) | Ch 4 §4.7 |
| [dup.md](dup.md) | dup (41), dup2 (63), dup3 (330) | Ch 5 §5.5 |
| [writev.md](writev.md) | writev (146), readv (145) | Ch 5 §5.7 |
| [fsync.md](fsync.md) | fsync (118) | Ch 13 §13.3 |
| [ioctl.md](ioctl.md) | ioctl (54) | Ch 4 §4.8 |

## Files and directories — TLPI Ch 15, 18

| File | Syscalls | TLPI |
|---|---|---|
| [stat.md](stat.md) | stat (106), fstat (108), stat64 (195), fstat64 (197) | Ch 15 §15.1 |
| [access.md](access.md) | access (33) | Ch 15 §15.4.3 |
| [unlink.md](unlink.md) | unlink (10) | Ch 18 §18.3 |
| [rename.md](rename.md) | rename (38) | Ch 18 §18.4 |
| [mkdir.md](mkdir.md) | mkdir (39), rmdir (40) | Ch 18 §18.6 |
| [readlink.md](readlink.md) | readlink (85) | Ch 18 §18.5 |
| [getdents.md](getdents.md) | getdents (141), getdents64 (220) | Ch 18 §18.8 |
| [chdir.md](chdir.md) | chdir (12), getcwd (183) | Ch 18 §18.10 |

## Processes and signals — TLPI Ch 6, 20–27

| File | Syscalls | TLPI |
|---|---|---|
| [getpid.md](getpid.md) | getpid (20), getppid (64), gettid (224) | Ch 6 §6.1 |
| [fork.md](fork.md) | fork (2) | Ch 24 |
| [exit.md](exit.md) | exit (1), exit_group (252) | Ch 25 |
| [waitpid.md](waitpid.md) | waitpid (7), wait4 (114) | Ch 26 |
| [execve.md](execve.md) | execve (11) | Ch 27 |
| [kill.md](kill.md) | kill (37) | Ch 20 §20.5 |
| [rt_sigaction.md](rt_sigaction.md) | rt_sigaction (174) | Ch 20–21 |

## Memory — TLPI Ch 7, 49, 50

| File | Syscalls | TLPI |
|---|---|---|
| [brk.md](brk.md) | brk (45) | Ch 7 §7.1 |
| [mmap.md](mmap.md) | mmap2 (192), mmap (90), munmap (91) | Ch 49 |
| [mprotect.md](mprotect.md) | mprotect (125) | Ch 50 §50.1 |
| [futex.md](futex.md) | futex (240) | Ch 30 (pthreads layer) |

## Time — TLPI Ch 10, 23

| File | Syscalls | TLPI |
|---|---|---|
| [time.md](time.md) | time (13), gettimeofday (78), clock_gettime (265) | Ch 10, Ch 23 §23.5 |
| [nanosleep.md](nanosleep.md) | nanosleep (162) | Ch 23 §23.4 |

## Pipes and sockets — TLPI Ch 44, 56–61, 63

| File | Syscalls | TLPI |
|---|---|---|
| [pipe.md](pipe.md) | pipe (42), pipe2 (331) | Ch 44 |
| [socket.md](socket.md) | socket (359) | Ch 56 |
| [bind.md](bind.md) | bind (361), setsockopt (366) | Ch 56 §56.3, Ch 59 |
| [listen.md](listen.md) | listen (363) | Ch 56 §56.5.1 |
| [accept4.md](accept4.md) | accept4 (364) | Ch 56 §56.5.2, Ch 60 |
| [connect.md](connect.md) | connect (362) | Ch 56 §56.4 |
| [socketcall.md](socketcall.md) | socketcall (102) — legacy entry point | Ch 56 |
| [poll.md](poll.md) | poll (168) | Ch 63 §63.2 |

## System info and misc

| File | Syscalls | TLPI |
|---|---|---|
| [uname.md](uname.md) | uname (122) | Ch 12 §12.1 |
| [getrandom.md](getrandom.md) | getrandom (355) | post-dates the book |

## Suggested reading order

1. `open` → `read` → `write` → `close` → `lseek` (the universal I/O model)
2. `exit` → `fork` → `waitpid` → `execve` (the process lifecycle)
3. `stat` → `unlink` → `mkdir` → `getdents` (the filesystem)
4. `dup` → `pipe` (how shells plumb pipelines)
5. `brk` → `mmap` (where memory comes from)
6. `socket` → `bind` → `listen` → `accept4` → then [../final_project.md](../final_project.md)
