#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Sep. 15, 2011
#

DIR_SOURCE := iptables-1.4.20
DIR_OUTPUT := $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all:
	cd $(DIR_SOURCE); \
	[ -f Makefile ] || ./autogen.sh; \
	[ -f Makefile ] || ./configure \
		--prefix=$(DIR_OUTPUT)\
		--exec-prefix=$(DIR_OUTPUT)\
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--with-ksource=$(DIR_LINUX) \
		--enable-static \
		--disable-shared \
		--disable-ipv6; \
	$(MAKE);\
	$(MAKE) install;

romfs:
	$(ROMFSINST) $(DIR_OUTPUT)/sbin/xtables-multi /bin/iptables

clean:
	[ ! -f $(DIR_SOURCE)/Makefile ] || $(MAKE) -C $(DIR_SOURCE) distclean;
