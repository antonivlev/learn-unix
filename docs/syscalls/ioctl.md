[← Back to index](README.md)

# ioctl (syscall 54)

The "everything else" call: device-specific operations on a file descriptor.

## What it does

`ioctl` covers operations that don't fit the read/write byte-stream model: terminal settings, getting the window size, ejecting a CD, configuring a network interface. You pass a request code (which selects the operation *and* implies the device type) and usually a pointer to a struct that the kernel reads or fills in. Each driver defines its own request codes — see `man ioctl_tty`, `man ioctl_list`. A few common ones on Linux i386:

| Request | Value | Meaning | `arg` |
|---|---|---|---|
| `TCGETS` | 0x5401 | Read the terminal's settings (what `tcgetattr` uses) | `struct termios *` (kernel fills it) |
| `TCSETS` | 0x5402 | Change the terminal's settings now, e.g. turn off echo or line buffering (`tcsetattr`) | `const struct termios *` |
| `TIOCGWINSZ` | 0x5413 | Get the terminal window size | `struct winsize *` (kernel fills it) |
| `TIOCSWINSZ` | 0x5414 | Set the terminal window size (terminal emulators do this; the foreground job gets `SIGWINCH`) | `const struct winsize *` |
| `FIONREAD` | 0x541B | How many bytes are ready to `read` right now (terminals, pipes, sockets) | `int *` (kernel stores the count) |
| `FIONBIO` | 0x5421 | Turn non-blocking mode on (`*arg` ≠ 0) or off (`*arg` = 0) | `const int *` |

`TCGETS` on an fd that isn't a terminal fails with `-ENOTTY` — that's how `isatty()` works.

## How it works

```c
int ioctl(int fd, unsigned long request, void *arg);
```

| Register | Parameter | Meaning |
|---|---|---|
| `eax` | — | syscall number 54 |
| `ebx` | `fd` | **In.** File descriptor of the device to control: e.g. 0, 1 or 2 when they are a terminal, or a number returned by `open("/dev/...")` or `socket` |
| `ecx` | `request` | **In.** Request code that selects the operation; each driver defines its own. See the table above |
| `edx` | `arg` | **In**, **Out** or **In/out**, depending on `request`: usually the address of a struct or `int` that the kernel reads (set requests) or fills in (get requests). Size and layout are fixed by the request; see the table above and the structs below. A few requests take a plain integer here, and some ignore it |

The two structs used by the requests above, as the kernel lays them out on i386:

`struct winsize` (8 bytes), used by `TIOCGWINSZ` / `TIOCSWINSZ`:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 2 | `ws_row` | Height in character rows |
| 2 | 2 | `ws_col` | Width in character columns |
| 4 | 2 | `ws_xpixel` | Width in pixels (often 0, unused) |
| 6 | 2 | `ws_ypixel` | Height in pixels (often 0, unused) |

`struct termios` (36 bytes, the kernel's version; glibc's `struct termios` is larger and libc converts), used by `TCGETS` / `TCSETS`:

| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 4 | `c_iflag` | Input mode bits (e.g. CR→NL translation) |
| 4 | 4 | `c_oflag` | Output mode bits (e.g. NL→CR NL translation) |
| 8 | 4 | `c_cflag` | Control bits: baud rate, character size, parity |
| 12 | 4 | `c_lflag` | Local mode bits: `ECHO` = 0x8 (echo typed keys), `ICANON` = 0x2 (line buffering), `ISIG` = 0x1 (Ctrl-C sends signals) |
| 16 | 1 | `c_line` | Line discipline number (0 for normal terminals) |
| 17 | 19 | `c_cc[19]` | Special characters (Ctrl-C, Ctrl-D, …) and the `VMIN`/`VTIME` settings for non-canonical reads |

The usual pattern is `TCGETS` to read the current settings, change a bit or two (e.g. clear `ECHO` in `c_lflag` to hide a password), then `TCSETS` with the same struct.

Return in `eax`: 0 on success for most requests (the result, if any, is written through `arg`); a few requests return a non-negative value instead. Negative means `-errno`.

Common errors: `-ENOTTY` (-25, this fd's device doesn't support that request — the classic "not a typewriter"), `-EBADF` (-9).

## Example

Get the terminal size with `TIOCGWINSZ`, which fills a `struct winsize` (four 16-bit fields: rows, cols, xpixel, ypixel).

```nasm
; nasm -f elf32 winsz.asm && i686-linux-gnu-ld -o winsz winsz.o && qemu-i386-static ./winsz
; Exits with the number of terminal rows: ./winsz; echo $?
section .bss
    ws resb 8               ; struct winsize: 4 x u16

section .text
global _start
_start:
    mov eax, 54         ; ioctl
    mov ebx, 1          ; fd 1 = stdout (must be a terminal)
    mov ecx, 0x5413     ; TIOCGWINSZ
    mov edx, ws         ; kernel fills this struct
    int 0x80

    movzx ebx, word [ws]    ; ws_row (first field)
    mov eax, 1              ; exit(rows)
    int 0x80
```

```c
#include <sys/ioctl.h>
struct winsize ws;
ioctl(1, TIOCGWINSZ, &ws);
printf("%d rows x %d cols\n", ws.ws_row, ws.ws_col);
```

## Linux philosophy

`ioctl` is the honest escape hatch of "everything is a file": devices mostly behave like files, but real hardware has knobs that bytes can't express, and cramming them into one grab-bag call keeps the core API (open/read/write/close) small and uniform. It's often criticized as unstructured — which is exactly the trade-off: uniformity for the common case, a typed-by-convention side channel for the rest.

## In The Linux Programming Interface

Ch 4 §4.8 introduces `ioctl` as the general mechanism; Ch 62 "Terminals" covers the terminal ioctls in depth, including `TIOCGWINSZ` and the `SIGWINCH` signal sent when the window is resized.
