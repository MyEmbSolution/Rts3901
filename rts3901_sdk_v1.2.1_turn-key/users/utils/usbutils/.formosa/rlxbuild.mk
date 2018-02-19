#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Aug. 21, 2014
#

BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all usbutils

all: usbutils

usbutils:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		LIBS=-liconv \
		PKG_CONFIG_PATH=$(DIR_TMPFS)/lib/pkgconfig; \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(ROMFSINST) $(BUILD)/bin/lsusb /bin/lsusb
	$(ROMFSINST) $(BUILD)/share/usb.ids /usr/share/usb.ids
	$(ROMFSINST) $(BUILD)/bin/usb-devices /bin/usb-devices

clean:
	rm -rf $(BUILD)
