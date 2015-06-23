#!/bin/sh
PKG_CFG_INCL=`pkg-config --cflags-only-I glib-2.0 ncurses`
PKG_CFG_LIBS=`pkg-config --libs-only-l glib-2.0 ncurses`

rm -rf vshell
gcc -o vshell $PKG_CFG_INCL -ggdb3 *.c $PKG_CFG_LIBS -lvterm -lutil


