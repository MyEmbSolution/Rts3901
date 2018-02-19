#
# Realtek Semiconductor Corp.
#
# Ming Qian (ming_qian@realsil.com.cn)
#
BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

ifeq ($(CONFIG_dbgutils_test_iodma), y)
	IODMA_FLAG ?= yes
else
	IODMA_FLAG ?= no
endif

.PHONY: all target

all: target

target:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake \
		-DDBGUTILS_IODMA_FLAG=$(IODMA_FLAG); \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin

clean:
	rm -rf $(BUILD)

