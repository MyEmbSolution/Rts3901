#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Dec. 18, 2014
#

BUILD := $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(BUILD)\
		--exec-prefix=$(BUILD)\
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--with-ksource=$(DIR_LINUX) \
	$(MAKE);\
	$(MAKE) install;

romfs:
	$(ROMFSINST) $(BUILD)/sbin/ethtool /bin/ethtool

clean:
	rm -rf $(BUILD)
