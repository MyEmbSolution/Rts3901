CROSS := mips-linux-
CC := $(CROSS)gcc
AR := $(CROSS)ar
RANLIB := $(CROSS)ranlib
CFLAGS := -Wall -I./include

OBJECTS = libmtd.a libmtd.so
all: $(OBJECTS) romfs tmpfs

libmtd_legacy.o: libmtd_legacy.c
	$(CC) $(CFLAGS) -c -o $@ $^
libmtd.o: libmtd.c
	$(CC) $(CFLAGS) -c -o $@ $^
libmtdops.o: libmtdops.c
	$(CC) $(CFLAGS) -c -o $@ $^

libmtd.a: libmtd_legacy.o libmtd.o libmtdops.o
	$(AR) cr $@ $^

libmtd.so: libmtd_legacy.c libmtd.c libmtdops.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^

romfs:
	$(ROMFSINST) libmtd.so /lib

tmpfs:
	$(TMPFSINST) include/libmtd.h /include/
	$(TMPFSINST) include/libmtdops.h /include/
	$(TMPFSINST) include/mtd_swab.h /include/
	$(TMPFSINST) libmtd.a /lib
	$(TMPFSINST) libmtd.so /lib

clean:
	-rm -f $(OBJECTS) *.o *.a *.so
