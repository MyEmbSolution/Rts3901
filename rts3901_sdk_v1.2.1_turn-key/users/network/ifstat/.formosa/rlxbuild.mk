#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 9, 2015
#
#
#

CROSS_TARGET ?= mips-linux

.PHONY: apps romfs clean

all: apps

apps:
	[ -f Makefile ] || \
	./autogen.sh --host=$(CROSS_TARGET); \
	$(MAKE)

romfs:
	$(ROMFSINST) ifstat /bin/ifstat

clean:
	$(MAKE) clean
