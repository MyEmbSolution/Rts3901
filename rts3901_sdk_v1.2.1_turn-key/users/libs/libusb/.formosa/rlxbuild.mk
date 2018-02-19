#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Aug. 21, 2014
#

BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all libusb

all: libusb tmpfs

libusb:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET); \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	for x in $(shell cd $(BUILD)/lib; ls *.so*); do \
		if [ -L $(BUILD)/lib/$$x ]; then \
			dest=`readlink $(BUILD)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/lib/$$x /lib; \
		fi; \
	done

clean:
	rm -rf $(BUILD)
