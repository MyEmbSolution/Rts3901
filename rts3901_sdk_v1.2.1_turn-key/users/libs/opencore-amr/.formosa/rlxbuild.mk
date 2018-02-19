#
# Realtek Semiconductor Corp.
#
# Tony Nie (tony_nie@realsil.com.cn)
# Jul. 13, 2015
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

BUILD = $(shell pwd)/.formosa/build
DIR_SOURCE = $(shell pwd)

.PHONY: amr tmpfs romfs clean
all: amr tmpfs

amr:
	mkdir -p $(BUILD)
	cd $(DIR_SOURCE); \
	[ -f Makefile ] || \
	  ./configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--enable-shared; \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	mkdir -p $(DIR_TMPFS)/include/opencore-amrnb/
	mkdir -p $(DIR_TMPFS)/include/opencore-amrwb/
	$(TMPFSINST) $(BUILD)/include/opencore-amrnb/interf_dec.h /include/opencore-amrnb/
	$(TMPFSINST) $(BUILD)/include/opencore-amrnb/interf_enc.h /include/opencore-amrnb/
	$(TMPFSINST) $(BUILD)/include/opencore-amrwb/dec_if.h /include/opencore-amrwb/
	$(TMPFSINST) $(BUILD)/include/opencore-amrwb/if_rom.h /include/opencore-amrwb/
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
