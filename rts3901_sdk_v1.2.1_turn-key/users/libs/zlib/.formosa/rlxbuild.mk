#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Mar. 19, 2015
#

BUILD = $(shell pwd)/.formosa/build
DIR_SOURCE = $(shell pwd)

.PHONY: all zlib

all: zlib tmpfs

zlib:
	mkdir -p $(BUILD)
	[ -f Makefile ] || \
	  ./configure \
		--prefix=$(BUILD); \
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
