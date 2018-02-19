#
# Realtek Semiconductor Corp.
#
# Ming Qian (ming_qian@realsil.com.cn)
#
BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all ft2_test

all: ft2_test

ft2_test:
	mkdir -p $(BUILD)
	cd $(BUILD); \
#	[ -f Makefile ] || \
	  cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake \
		-DFT2_TEST_LOOP=$(CONFIG_ft2_test_loop); \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(ROMFSINST) $(BUILD)/bin /usr/bin

clean:
	rm -rf $(BUILD)

