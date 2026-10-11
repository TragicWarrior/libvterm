# Introduction #

libvtty is a terminal emulator library for embedding a working VT
implementation inside another program.  It descends from ROTE and
emulates the VT100, rxvt, xterm, xterm-256color, and Linux console
modes.  As of 10.0 it also supports 24-bit truecolor and a tunable
RGB-to-xterm color cache.  The natural display apparatus is curses,
but the library can also drive a stream buffer for output (a
contribution from Hitachi-ID).

libvtty was called libvterm until version 11.0.  If you are looking
for libvterm by Bryan Christ, this is it; see [History](#history) for
why the name changed and [Migrating from libvterm](#migrating-from-libvterm)
for what to change in a program that uses it.

# Features #

* VT100, rxvt, xterm, xterm-256color, and Linux console emulation
* 24-bit truecolor (via `--truecolor` on vshell or VTTY_FLAG_TRUECOLOR
  on the API) with COLORTERM=truecolor exported into the child
* 256-color mode with RGB-to-xterm cube/grayscale mapping and a
  pair_select callback so integrators can pin color pairs
* Per-instance OSC 4 palette isolation -- palette tweaks in one vtty
  don't bleed into a sibling
* UTF-8 throughout, with a fast direct-byte-math decoder
* Multi-buffer support (standard + alternate) with scrollback /
  history on the standard buffer
* Mouse: X10 and SGR protocols; left, middle and right buttons with
  Shift / Alt / Ctrl; the wheel; drag and hover reporting (DECSET 1002
  and 1003); GPM wheel forwarding; and vtty_write_mouse_event for
  embedder-driven injection
* Window titles: a program's title (OSC 0 / OSC 2) is kept for
  vtty_get_title and announced with VTTY_EVENT_TITLE when it changes
* OSC 52 clipboard SET from the child (inner hop): decoded payload is
  stored and delivered via VTTY_EVENT_CLIPBOARD.  The library does
  not talk to the host clipboard; the embedder decides xclip / OSC 52
  out / ignore.  Query (`Pd = ?`) is refused.
* Async I/O interface (SIGIO + self-pipe trick) for callers that
  don't want to spin a thread
* Optional crash handler (`ENABLE_BACKTRACE`) that writes a glibc
  backtrace to `./vtty-crash-<pid>.log` for post-mortem debugging
* Link-time optimization auto-enabled when the toolchain supports it

# Requirements #

* A working compiler
  * GCC 4.5 or later on Linux
  * Clang on Mac OS and FreeBSD
* ncursesw (the "wide" build of ncurses) and headers installed in a
  reasonably sane location (see Mac OS notes)
* CMake 3.10.2 or later

The build system is CMake.  In order to properly find and link
against ncursesw, your CMake must not be too old.  3.10.2 (January
2018) is the oldest version that has been tested successfully; older
versions _might_ work, and you can edit `CMakeLists.txt` to lower
the minimum if you want to try.

When the version of CMake provided by your OS vendor is too old, it
is straightforward to install a newer one from source and place it in
`/usr/local/bin/` or another alternate location.

# Building #

A standard build goes something like this:

```
cmake CMakeLists.txt
make
sudo make install
```

A successful build installs the shared library to `/usr/local/lib` and
the `vshell` demo program to `/usr/local/bin/`.  On Linux the install
then runs `ldconfig` to refresh the linker cache (a staged install with
`DESTDIR` skips it).  If your runtime linker configuration (typically
`/etc/ld.so.conf`) isn't configured to look in `/usr/local/lib`, you
will still need to add it and run `ldconfig` yourself.

`vshell` is the canonical demo: it embeds libvtty into a tmux-like
front end and exposes the major API surfaces (scrollback, mouse,
title, resize, color modes, OSC 52 clipboard).  A child copy
(`OSC 52`) is taken by the event hook and forwarded with xclip plus
a best-effort OSC 52 to the outer terminal; the frame shows
`CLIP <nbytes>`.  Useful flags include `--truecolor`, `--c16`,
`--no-utf8`, `--vt100`, `--scrollback <lines>` (64 - 512), and
`--exec <program>`.

## Using the library ##

A program includes one header and links one library:

```
#include <vtty.h>
```

```
cc -o myterm myterm.c -lvtty -lncursesw
```

`pkg-config --libs libvtty` gives the same linker flag.  The shared
library carries a versioned soname (`libvtty.so.11`), so a program
built against one major version is not loaded against another.

The `API` file describes the core calls and `vtty.h` is the complete
reference.  `demo/vshell.c` is a full working host.

## FreeBSD ##

CMake does not detect the ncurses or ncursesw library correctly on
FreeBSD, so the build check is conditionally excluded there.  As a
result the build expects ncurses headers at
`/usr/local/include/ncurses` (the ports install location).

## Mac OS ##

Porting was done on Mojave; mileage on other versions may vary.  The
ncurses that ships with XCode lacks wide-character support, which
libvtty needs for UTF-8.  The build is therefore wired to look in:

```
/usr/local/opt/ncurses/
```

The easiest way to get a working ncursesw there is to install `htop`
via Homebrew.

## Debian Packages ##

If you want to build a `.deb` for Debian or a Debian-based system
(Ubuntu, etc.):

```
sudo make package
```

# Performance Tuning #

A terminal emulator should be light and fast.  Where it matters the
hot paths in libvtty are written (or re-written) for efficiency.  By
default the build enables `gcc -O2`; `-O3` has been reasonably tested
and yields roughly a 2% CPU improvement -- it may become the default
in the future.

Link-time optimization (LTO) is auto-enabled when the toolchain
supports it (CMake's `CheckIPOSupported` probe).  LTO lets the
compiler inline across translation-unit boundaries, which matters
for the many small cross-TU helpers in libvtty.  It is automatically
disabled when `ENABLE_BACKTRACE` is set, since that mode forces `-O0`
for predictable stack frames.

# History #

The library has had three names.

## ROTE (2004) ##

The code began as ROTE, "Our Own Terminal Emulation library", written
by Bruno Takahashi C. de Oliveira.  ROTE was a small C library that
emulated a VT102 and drew it with curses.

## libvterm (circa 2006 - 2026) ##

Bryan Christ began libvterm around 2006.  He later took over the
maintenance of ROTE, with its author's permission, and the ROTE code
became the base of a new libvterm: a fork of ROTE that carried on
under the libvterm name.  That project was registered on SourceForge
in October 2009, and ROTE's own page came to recommend libvterm for
new work.

Under that name the library grew from a VT102 into what it is now:
xterm, 256-color and truecolor emulation, UTF-8, an alternate screen,
scrollback, mouse reporting and clipboard support.

The project moved from SourceForge to Google Code, and in June 2015
to GitHub.  In 2017 Hitachi-ID contributed the ability to run the
emulator with no pty and no curses, rendering into a stream buffer.

## libvtty (2026 - ) ##

There is another, unrelated terminal emulator library called libvterm:
the one written by Paul Evans and hosted on Launchpad, which Neovim
and Emacs use and which most Linux distributions package.  The two
libraries share no code, but both installed `libvterm.so` and
`vterm.h`, so they could not be installed side by side and this one
could not be packaged under its own name.

Version 11.0, in October 2026, renamed this project to libvtty to end
that collision.  The rename changed names only: 11.0.0 behaves exactly
as libvterm 10.10.2 did.

The `CHANGELOG` keeps the full record.  Its entries before 11.0.0 use
the names that were current when they were written.

## Migrating from libvterm ##

Nothing was removed or redesigned; every public name changed its
prefix, and there are no aliases for the old names.

| | libvterm 10 | libvtty 11 |
|---|---|---|
| Header | `vterm.h` | `vtty.h` |
| Linker flag | `-lvterm` | `-lvtty` |
| Shared library | `libvterm.so` | `libvtty.so.11` |
| pkg-config | `libvterm.pc` | `libvtty.pc` |
| Functions | `vterm_*` | `vtty_*` |
| Types | `vterm_t`, `vterm_cell_t` | `vtty_t`, `vtty_cell_t` |
| Callback types | `VtermEventHook` | `VttyEventHook` |
| Macros | `VTERM_*` | `VTTY_*` |
| Version string | `LIBVTERM_VERSION` | `LIBVTTY_VERSION` |

For most programs the whole migration is a case-preserving
search-and-replace of `vterm` with `vtty`:

```
sed -i -e 's/vterm/vtty/g' -e 's/VTERM/VTTY/g' -e 's/Vterm/Vtty/g' *.c *.h
```

Read the result before committing it.  The replacement also rewrites
any name of your own that contains "vterm", which you may not want.

Installing libvtty does not remove an installed libvterm.  Delete
`libvterm.so`, `libvterm.a`, `vterm.h` and `libvterm.pc` by hand.
