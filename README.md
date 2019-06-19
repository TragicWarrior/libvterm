# Introduction #

Based on ROTE, libvterm is a terminal emulator library which attempts to
mimic VT100, rxvt, xterm and xterm 256 color capabilities. Although the
natural display apparatus is curses, current contributions from Hitachi-ID
have allowed it to use a stream buffer for output.

# Requirements

* A working compiler
  *  GCC version 4.5 or higher for Linux
  *  Clang for Mac OS and FreeBSD
* The "wide" version of ncurses (ncursesw) and headers installed in a
  reasonably sane location see notes on Mac OS).
* CMake 3.10.2 or higher.

The build system for libvterm is CMake.  In order to properly find and link
against ncursesw, CMake must be too old.  The oldest version that has
been tested successfully is 3.10.2 (which was released in January of 2018).
Older versions _might_ work.  If you edit the CMakeList.txt you can change
the minimum requirement for CMake version.

When the version of CMake provided by your OS vendor is too old, it is
relatively easy to install a new version from source and place it in
/usr/local/bin/ or some other alternate location.  

# Building #

A standard build goes something like this:

```
cmake CMakeList.txt
make
sudo make install
```

A successful build will place the shared library in /usr/local/lib and the
demo program (vshell) in /usr/local/bin/.  If you your runtime linker
configuration (typically /etc/ld.so.conf) isn't configured to look in this
location you, will need to manually add it and update the linker cache
(typically running ldconfig). 

## FreeBSD ##

CMake does not detect the ncurses or ncurses wide library correctly on
FreeBSD.  Therefore, the CMake build check for ncurses is conditionally
excluded on FreeBSD.  As a result, the build process on FreeBSD blindly
expects ncurses headers to be found at /usr/local/include/ncurses (which
is where it gets installed via ports).

## Mac OS ##

Porting of libvterm was done on Mojave.  Mileage on other version of
Mac OS may vary.  The version of ncurses which comes with XCode does
not include support for wide characters which libvterm needs to supprt
UTF-8 encoded Unicode.  As a result, the build environment is designed
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
