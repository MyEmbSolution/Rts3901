#
# Realtek Semiconductor Corp.
#
# Jun-Ru Chang (jrjang@realtek.com)
# May. 29, 2012
#

BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all $(POPT)

all: popt tmpfs

popt:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	    ac_cv_func_iconv=no \
	    am_cv_func_iconv=no \
	    am_cv_func_iconv_works=no \
	    am_cv_lib_iconv=no \
	    ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--disable-nls; \
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
