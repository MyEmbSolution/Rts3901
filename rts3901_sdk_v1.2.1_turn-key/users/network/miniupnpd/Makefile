CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CFLAGS += -Wall -g -I.
CFLAGS += -I$(DIR_TMPFS)/include -shared-libgcc
LDLIBS += -lrtsnm -lm
LDFLAGS += -L$(DIR_TMPFS)/lib

BASEOBJS = miniupnpd.o upnphttp.o upnpdescgen.o upnpsoap.o \
           upnpreplyparse.o minixml.o \
           getifaddr.o daemonize.o upnpglobalvars.o \
           options.o upnppermissions.o minissdp.o \
           upnpevents.o upnputils.o asyncsendto.o \
	   getconnstatus.o getifstats.o ifacewatcher.o

EXECUTABLES = miniupnpd

.PHONY:	all clean romfs depend

all:	$(EXECUTABLES)

miniupnpd:	$(BASEOBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

depend:	config.h
	makedepend -f$(MAKEFILE_LIST) -Y \
	$(ALLOBJS:.o=.c) $(TESTUPNPDESCGENOBJS:.o=.c) \
	testgetifstats.c  testupnppermissions.c testgetifaddr.c \
	miniupnpdctl.c 2>/dev/null
clean:
	rm -rf $(EXECUTABLES)
	rm -rf *.o
romfs:

	$(ROMFSINST) ./miniupnpd /bin/miniupnpd
