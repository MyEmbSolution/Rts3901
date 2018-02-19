#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# Aug. 4, 2015
#
CROSS_TARGET ?= mips-linux
.PHONY: all tcpdump romfs clean

all: tcpdump

tcpdump:
	[ -f Makefile ] || \
	./configure --host=$(CROSS_TARGET) --disable-ipv6 ac_cv_linux_vers=3; \
	make

romfs:
	$(ROMFSINST) tcpdump /bin/tcpdump

clean:
	$(MAKE) clean
