[← Back to index](README.md)

# execve (syscall 11)

Replace the current process image with a new program.

## What it does
`execve` throws away the calling process's code, data, and stack, and loads a new program in their place. The PID, open file descriptors, and parent stay the same — it's still the same process, just running different code. On success it **never returns**; if you see a return value, the call failed. Both `argv` and `envp` are arrays of `char*` pointers terminated by a NULL entry.

## How it works
```c
int execve(const char *pathname, char *const argv[], char *const envp[]);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 11 |
| `ebx` | `pathname` | **In.** Address of a NUL-terminated string naming the file to run, e.g. `"/bin/ls", 0`. It must be an executable ELF binary or a script starting with `#!`. A relative path is resolved against the current directory |
| `ecx` | `argv` | **In.** Address of an array of 4-byte pointers, one per command-line argument, each pointing to a NUL-terminated string, ending with a 0 (NULL) entry. The new program receives these as its `argc`/`argv`. 0 is accepted as an empty list (since Linux 5.18 the kernel then supplies a single empty string as `argv[0]`) |
| `edx` | `envp` | **In.** Same shape as `argv`, but each string has the form `"NAME=value"`; these become the new program's environment variables. 0 = empty environment |

What `argv` and `envp` look like in memory (each slot is a 4-byte address on i386):

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `argv[0]` | address of the program-name string, e.g. `"/bin/ls"` |
| 4 | 4 | `argv[1]` | address of the first real argument, e.g. `"-l"` |
| 8 | 4 | `argv[2]` | address of the next argument, e.g. `"/tmp"` |
| 12 | 4 | NULL | 0 marks the end of the list; the kernel counts entries up to here to get `argc` |

In NASM, for `ls -l /tmp` with the environment `LANG=C`:

```nasm
section .data
    path  db "/bin/ls", 0
    arg1  db "-l", 0
    arg2  db "/tmp", 0
    argv  dd path, arg1, arg2, 0    ; argv[0], argv[1], argv[2], NULL
    env0  db "LANG=C", 0
    envp  dd env0, 0                ; envp[0], NULL
```

`pathname` is used as given, with no `$PATH` search: a name without a `/` is looked up in the current directory (libc's `execvp` does the `$PATH` search). By convention `argv[0]` is the program's name. Passing 0 for `envp` gives the new program an empty environment (Linux treats a NULL `envp` as an empty list); to pass your environment on, give it the `envp` array your program found on its startup stack.

Return in `eax`: on success, nothing — the old program is gone and the new one starts at its entry point with `argc`, `argv` and `envp` laid out on its fresh stack. A return means failure and `eax` holds `-errno`: `-ENOENT` (-2) — no such file; `-EACCES` (-13) — not executable; `-EFAULT` (-14) — a bad pointer in `pathname`, `argv` or `envp`; `-E2BIG` (-7) — the argument and environment strings are too large.

## Example
The classic pattern: fork a child, exec in the child, wait in the parent.

```nasm
; nasm -f elf32 exec.asm && i686-linux-gnu-ld -o exec exec.o && qemu-i386-static ./exec
section .data
    path db "/bin/ls", 0
    argv dd path, 0     ; char *argv[] = { "/bin/ls", NULL }

section .text
global _start
_start:
    mov eax, 2          ; fork()
    int 0x80
    test eax, eax
    jnz .parent

    ; child: execve("/bin/ls", argv, NULL)
    mov eax, 11
    mov ebx, path
    mov ecx, argv
    xor edx, edx        ; envp = NULL
    int 0x80
    mov eax, 1          ; only reached if execve failed
    mov ebx, 127
    int 0x80

.parent:
    mov ebx, eax        ; waitpid(child, NULL, 0)
    mov eax, 7
    xor ecx, ecx
    xor edx, edx
    int 0x80
    mov eax, 1          ; exit(0)
    xor ebx, ebx
    int 0x80
```

## Linux philosophy
Exec is the other half of the fork/exec split: fork makes a process, exec fills it with a program. Because they're separate, the child can rearrange its world first — redirect stdout to a file, set up a pipe — using nothing but ordinary syscalls. That tiny gap between fork and exec is where all of shell redirection and piping lives.

## In The Linux Programming Interface
Ch 27 "Program Execution" (p.563). The book covers the whole `exec` family of libc wrappers (`execlp`, `execvp`, ...), `#!` interpreter scripts, and what execve preserves versus resets.
