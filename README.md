## Introduction ##

Based on ROTE, libvterm is a terminal emulator library which attempts to
mimic VT100, rxvt, xterm and xterm 256 color capabilities. Although the
natural display apparatus is curses, current contributions from Hitachi-ID
have allowed it to use a stream buffer for output.

## Building ##

The build system for libvterm uses CMake and it automatically detects a
variety of different settings and libraries.  A default build tries to
include support for curses/ncurses.  If you want to suppress the behavior,
specify the following:

cmake -DDEFINE_CURSES=OFF

A standard build goes something like this:

cmake CMakeList.txt
make
sudo make install

If you want to build a debian package then:

sudo make package

## Performance Tuning ##

A terminal emulation should be light and fast.  Whenever possible, the
routines in libvterm are written (or re-written) to be highly efficient.
When the library is compiled, gcc -O2 optimizations are enabled.  The
library has been reasonably tested with -O3 optimizations enabled.  Turning
these on should result in about a 2-percent improvement in CPU usage.  In
the future -O3 might become the default.
