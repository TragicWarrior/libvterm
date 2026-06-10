# Introduction #

libvterm is a terminal emulator library for embedding a working VT
implementation inside another program.  It descends from ROTE and
emulates the VT100, rxvt, xterm, xterm-256color, and Linux console
modes.  As of 10.0 it also supports 24-bit truecolor and a tunable
RGB-to-xterm color cache.  The natural display apparatus is curses,
but the library can also drive a stream buffer for output (a
contribution from Hitachi-ID).

# Features #

* VT100, rxvt, xterm, xterm-256color, and Linux console emulation
* 24-bit truecolor (via `--truecolor` on vshell or VTERM_FLAG_TRUECOLOR
  on the API) with COLORTERM=truecolor exported into the child
* 256-color mode with RGB-to-xterm cube/grayscale mapping and a
  pair_select callback so integrators can pin color pairs
* Per-instance OSC 4 palette isolation -- palette tweaks in one vterm
  don't bleed into a sibling
* UTF-8 throughout, with a fast direct-byte-math decoder
* Multi-buffer support (standard + alternate) with scrollback /
  history on the standard buffer
* Mouse: X10 and SGR protocols, GPM wheel forwarding, and
  vterm_write_mouse_event for embedder-driven injection
* Async I/O interface (SIGIO + self-pipe trick) for callers that
  don't want to spin a thread
* Optional crash handler (`ENABLE_BACKTRACE`) that writes a glibc
  backtrace to `./vterm-crash-<pid>.log` for post-mortem debugging
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
the `vshell` demo program to `/usr/local/bin/`.  If your runtime
linker configuration (typically `/etc/ld.so.conf`) isn't configured to
look in this location, you will need to add it and refresh the linker
cache (typically `ldconfig`).

`vshell` is the canonical demo: it embeds libvterm into a tmux-like
front end and exposes the major API surfaces (scrollback, mouse,
title, resize, color modes).  Useful flags include `--truecolor`,
`--c16`, `--no-utf8`, `--vt100`, `--scrollback <lines>` (64 - 512),
and `--exec <program>`.

## FreeBSD ##

CMake does not detect the ncurses or ncursesw library correctly on
FreeBSD, so the build check is conditionally excluded there.  As a
result the build expects ncurses headers at
`/usr/local/include/ncurses` (the ports install location).

## Mac OS ##

Porting was done on Mojave; mileage on other versions may vary.  The
ncurses that ships with XCode lacks wide-character support, which
libvterm needs for UTF-8.  The build is therefore wired to look in:

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
hot paths in libvterm are written (or re-written) for efficiency.  By
default the build enables `gcc -O2`; `-O3` has been reasonably tested
and yields roughly a 2% CPU improvement -- it may become the default
in the future.

Link-time optimization (LTO) is auto-enabled when the toolchain
supports it (CMake's `CheckIPOSupported` probe).  LTO lets the
compiler inline across translation-unit boundaries, which matters
for the many small cross-TU helpers in libvterm.  It is automatically
disabled when `ENABLE_BACKTRACE` is set, since that mode forces `-O0`
for predictable stack frames.
