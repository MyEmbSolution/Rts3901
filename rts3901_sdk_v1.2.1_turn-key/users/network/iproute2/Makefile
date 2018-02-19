#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Sep. 15, 2011
#

LIBDIR=/lib
DEFINES= -DRESOLVE_HOSTNAMES -DLIBDIR=\"$(LIBDIR)\"

LDLIBS=
ADDLIB=

#options for decnet
#ADDLIB+=dnet_ntop.o dnet_pton.o

#options for ipx
#ADDLIB+=ipx_ntop.o ipx_pton.o

HOSTCC = gcc
CCOPTS = -D_GNU_SOURCE -Os -Wstrict-prototypes -Wall
CFLAGS := $(CCOPTS) -I../include $(DEFINES) $(CFLAGS)
YACCFLAGS = -d -t -v

LDLIBS += -L../lib -lnetlink -lutil

#SUBDIRS=lib ip tc misc netem genl
SUBDIRS=lib ip tc

LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a

all:
	sh configure $(DIR_LINUX)/include
	@set -e; \
	for i in $(SUBDIRS); \
		do $(MAKE) -C $$i; \
	done

romfs:
	$(MAKE) -C tc romfs

clean:
	@for i in $(SUBDIRS) doc; \
		do $(MAKE) -C $$i clean; \
	done
	rm Config

.EXPORT_ALL_VARIABLES:
