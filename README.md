# Introduction #

Based on ROTE, libvterm is a terminal emulator library which attempts to
mimic VT100, rxvt, xterm and xterm 256 color capabilities. Although the
natural display apparatus is curses, current contributions from Hitachi-ID
have allowed it to use a stream buffer for output.

# Building #

A standard build goes something like this:

```
cmake CMakeList.txt
make
sudo make install
```

## FreeBSD ##

CMake does not detect the ncurses or ncurses wide library correctly on
FreeBSD.  Therefore, the CMake build check for ncurses is conditionally
excluded on FreeBSD.  As a result, the build process on FreeBSD blindly
expects ncurses headers to be found at /usr/local/include/ncurses (which
is where it gets installed via ports).

## Mac OS ##

Porting of libvterm was done on Mojave.  Mileage on other version of
Mac OS may vary.  The version of ncurses which comes with XCode does
not include support for wide characters which libterm needs to supprt
utf-8 encoded Unicode.  As a result, the build environment is designed
to look for the library in:

/usr/local/opt/ncurses/

The easiest way to get this is to install 'htop' via homembrew.

## Debian Packages ##

If you want to build a deb package for Debian or Debian based systems
(such as Ubuntu) then:

```
sudo make package
```

# Performance Tuning #

A terminal emulation should be light and fast.  Whenever possible, the
routines in libvterm are written (or re-written) to be highly efficient.
When the library is compiled, gcc -O2 optimizations are enabled.  The
library has been reasonably tested with -O3 optimizations enabled.  Turning
these on should result in about a 2-percent improvement in CPU usage.  In
the future -O3 might become the default.
