# makefile for libpng on Linux ELF with gcc
# Copyright (C) 1998, 1999 Greg Roelofs
# Copyright (C) 1996, 1997 Andreas Dilger
# For conditions of distribution and use, see copyright notice in png.h

#CC=gcc

# where "make install" puts libpng.a, libpng.so*, png.h and pngconf.h
prefix=$(shell pwd)/.formosa/build

ALIGN=

WARNMORE=-Wwrite-strings -Wpointer-arith -Wshadow \
	-Wmissing-declarations -Wtraditional -Wcast-align \
	-Wstrict-prototypes -Wmissing-prototypes #-Wconversion

# for pgcc version 2.95.1, -O3 is buggy; don't use it.

CFLAGS += -Wall -Os -funroll-loops $(ALIGN)

# read libpng.txt or png.h to see why PNGMAJ is 2.  You should not
# have to change it.
PNGMAJ = 2
PNGMIN = 1.0.12
PNGVER = $(PNGMAJ).$(PNGMIN)

OBJS = png.o pngset.o pngget.o pngrutil.o pngtrans.o pngwutil.o \
	pngread.o pngrio.o pngwio.o pngwrite.o pngrtran.o \
	pngwtran.o pngmem.o pngerror.o pngpread.o

OBJSDLL = $(OBJS:.o=.pic.o)

.SUFFIXES:      .c .o .pic.o

.c.pic.o:
	$(CC) -c $(CFLAGS) -fPIC -o $@ $*.c

all: libpng.a tmpfs

libpng.a: $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

libpng.so: libpng.so.$(PNGMAJ)
	ln -sf libpng.so.$(PNGMAJ) libpng.so

libpng.so.$(PNGMAJ): libpng.so.$(PNGVER)
	ln -sf libpng.so.$(PNGVER) libpng.so.$(PNGMAJ)

libpng.so.$(PNGVER): $(OBJSDLL)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libpng.so.$(PNGMAJ) \
		-o libpng.so.$(PNGVER) $(OBJSDLL) -lz -lm -lc

tmpfs: libpng.a
	$(TMPFSINST) /include/png.h
	$(TMPFSINST) /include/pngconf.h
	$(TMPFSINST) /lib/libpng.a

romfs:

clean:
	-/bin/rm -f *.o libpng.a libpng.so* pngout.png

# DO NOT DELETE THIS LINE -- make depend depends on it.
png.o png.pic.o: png.h pngconf.h
pngerror.o pngerror.pic.o: png.h pngconf.h
pngrio.o pngrio.pic.o: png.h pngconf.h
pngwio.o pngwio.pic.o: png.h pngconf.h
pngmem.o pngmem.pic.o: png.h pngconf.h
pngset.o pngset.pic.o: png.h pngconf.h
pngget.o pngget.pic.o: png.h pngconf.h
pngread.o pngread.pic.o: png.h pngconf.h
pngrtran.o pngrtran.pic.o: png.h pngconf.h
pngrutil.o pngrutil.pic.o: png.h pngconf.h
pngtrans.o pngtrans.pic.o: png.h pngconf.h
pngwrite.o pngwrite.pic.o: png.h pngconf.h
pngwtran.o pngwtran.pic.o: png.h pngconf.h
pngwutil.o pngwutil.pic.o: png.h pngconf.h
pngpread.o pngpread.pic.o: png.h pngconf.h
