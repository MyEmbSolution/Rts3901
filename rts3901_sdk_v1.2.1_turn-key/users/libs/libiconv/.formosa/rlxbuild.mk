#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

BUILD = $(shell pwd)/.formosa/build
DIR_SOURCE = $(shell pwd)

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)

all: libiconv tmpfs

libiconv:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--enable-static \
		--enable-shared; \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	for x in $(shell cd $(BUILD)/lib; ls libcharset*.so* libiconv*.so* preloadable_libiconv.so); do \
		if [ -L $(BUILD)/lib/$$x ]; then \
			dest=`readlink $(BUILD)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	$(Q)rm -rf $(BUILD)
