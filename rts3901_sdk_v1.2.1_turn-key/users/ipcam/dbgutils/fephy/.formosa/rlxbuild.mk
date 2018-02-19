#
# Realtek Semiconductor Corp.
#
# Ming Qian (ming_qian@realsil.com.cn)
#
BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all target

all: target

target:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake; \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin

clean:
	rm -rf $(BUILD)

