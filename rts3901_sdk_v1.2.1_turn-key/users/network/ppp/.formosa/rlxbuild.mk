#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 21, 2015
#

.PHONY: all ppp romfs clean

CROSS_TARGET ?= mips-linux
all: ppp

ppp:
	[ -f Makefile ] || \
	./configure; \
	make CC=$(CROSS_TARGET)-gcc

romfs:
	$(ROMFSINST) pppd/pppd /bin/pppd
	$(ROMFSINST) pppd/plugins/rp-pppoe/rp-pppoe.so /lib/ppp/rp-pppoe.so

clean:
	if [ -e Makefile ]; then \
		$(MAKE) clean; \
	fi
