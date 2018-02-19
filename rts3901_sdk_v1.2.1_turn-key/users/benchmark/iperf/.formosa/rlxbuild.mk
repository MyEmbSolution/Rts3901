#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Mar. 02, 2010
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

BUILD := $(shell pwd)/.formosa/build

.PHONY: apps romfs clean
all: apps

apps:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	    ../../configure \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--enable-threads \
		ac_cv_func_malloc_0_nonnull=yes; \
	$(MAKE)

romfs:
	$(ROMFSINST) $(BUILD)/src/iperf /bin/iperf

clean:
	rm -rf $(BUILD)
