#!/bin/sh
reset

cp ../libvterm.so .
cp ../vterm.h .

gcc -g\
    -L/home/idan/src/libvterm/test\
    -I/usr/include/glib-2.0\
    -I/usr/lib/x86_64-linux-gnu/glib-2.0/include\
    -o test\
    test.c\
    -lvterm\
    -lncurses\
    -lutil\
&& LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH ./test /tmp/idan
