#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
#
BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

ifeq ($(CONFIG_PACKAGE_nm_wps), y)
NM_WPS_FLAG ?= yes
else
NM_WPS_FLAG ?= no
endif

ifeq ($(CONFIG_FEATURE_IPV6), y)
IPV6_ENABLE ?= yes
else
IPV6_ENABLE ?= no
endif

.PHONY: all

all: target tmpfs

target:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake \
		-DDIR_TMPFS=$(DIR_TMPFS) \
		-DNM_WPS_FLAG=$(NM_WPS_FLAG) \
		-DCONFIG_FEATURE_IPV6=$(IPV6_ENABLE); \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	$(TMPFSINST) $(BUILD)/bin/testnm /bin

romfs:
	$(ROMFSINST) $(BUILD)/bin/testnm /bin

clean:
	rm -rf $(BUILD)

