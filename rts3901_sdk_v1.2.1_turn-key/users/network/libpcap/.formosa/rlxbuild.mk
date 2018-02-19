#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# Aug. 4, 2015
#
CROSS_TARGET ?= mips-linux
.PHONY: all libpcap romfs clean

all: libpcap

libpcap:
	[ -f Makefile ] || \
	./configure --host=$(CROSS_TARGET) --with-pcap=linux ac_cv_linux_vers=3; \
	make

clean:
	$(MAKE) clean
