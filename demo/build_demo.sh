#!/bin/sh
PKG_CFG_INCL=`pkg-config --cflags-only-I ncurses`
PKG_CFG_LIBS=`pkg-config --libs-only-l ncurses`

rm -rf vshell
gcc -o vshell $PKG_CFG_INCL -ggdb3 *.c $PKG_CFG_LIBS -lvterm -lutil


