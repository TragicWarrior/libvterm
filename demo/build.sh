#!/bin/sh
reset

cp ../libvterm.so .
cp ../vterm.h .

gcc -g\
    -L`pwd`\
    -I`pwd`\
    -o vshell\
    vshell.c\
    -lvterm\
    -lncurses\
    -lutil\
&& LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH ./vshell /tmp/idan
