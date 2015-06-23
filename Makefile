CFLAGS = -Wall -O2 -std=c99
DEFS = -D_REENTRANT -D_POSIX_C_SOURCE=200112L
prefix = /usr
includedir = ${prefix}/include

HOSTARCH = $(shell arch)

ifeq ($(HOSTARCH),x86_64)
LIB_ARCH = lib64
else
LIB_ARCH = lib
endif

makefile: all

all: libvterm

debug:
	gcc $(CFLAGS) $(DEFS) -D_DEBUG -c -fPIC *.c
	gcc -shared -o libvterm.so -lutil *.o

libvterm:
	gcc $(CFLAGS) $(DEFS) -c -fPIC *.c
	gcc -shared -o libvterm.so -lutil *.o

clean:
	rm -f *.o
	rm -f *.so

install:
	cp -f vterm.h $(includedir)
	chmod 755 libvterm.so
	cp -f libvterm.so ${prefix}/${LIB_ARCH}
	ldconfig	
	

