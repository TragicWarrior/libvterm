#!/bin/sh
reset

cp ../libvterm.so .
cp ../vterm.h .

FIFO=/tmp/`whoami`

if [ -p "$FIFO" ] ; then
  echo "FIFO exists"
else
  mkfifo $FIFO
fi

echo "Open another xterm of the same size and script -f $FIFO"
echo "Press ctrl-D when done to see the final screen here."

gcc -g\
    -L`pwd`\
    -I`pwd`\
    -o test\
    test.c\
    -lvterm\
    -lutil\
&& LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH ./test $FIFO
