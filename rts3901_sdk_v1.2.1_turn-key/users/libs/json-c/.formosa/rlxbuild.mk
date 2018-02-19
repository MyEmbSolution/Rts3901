#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Aug. 4, 2014
#

BUILD = $(shell pwd)/.formosa/build
DIR_SOURCE = $(shell pwd)

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all json

all: json tmpfs

json:
	mkdir -p $(BUILD)
	cd $(DIR_SOURCE); \
	[ -f configure ] || \
	  sh ./autogen.sh; \
	[ -f Makefile ] || \
	  ./configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--enable-shared; \
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
	if [ -e $(DIR_SOURCE)/Makefile ]; then \
		$(MAKE) -C $(DIR_SOURCE) clean; \
		rm -f $(DIR_SOURCE)/Makefile; \
	fi
