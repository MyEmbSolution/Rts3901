#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Nov. 19, 2014
#

BUILD = $(shell pwd)/.formosa/build
PREFIX = $(shell pwd)/.formosa/install

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all lzo

all: lzo tmpfs

lzo:
	mkdir -p $(BUILD)
	mkdir -p $(PREFIX)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(PREFIX) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET); \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	$(TMPFSINST) $(PREFIX)/include /include
	$(TMPFSINST) $(PREFIX)/lib /lib

romfs:
	for x in $(shell cd $(PREFIX)/lib; ls *.so*); do \
		if [ -L $(PREFIX)/lib/$$x ]; then \
			dest=`readlink $(PREFIX)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(PREFIX)/lib/$$x /lib; \
		fi; \
	done

clean:
	rm -rf $(BUILD)
	rm -rf $(PREFIX)
