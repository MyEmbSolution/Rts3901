#
# Realtek Semiconductor Corp.
#
# Tony Nie (tony_nie@realsil.com.cn)
# Aug. 14, 2014
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

BUILD := $(shell pwd)/.formosa/build

PCRE = pcre_target

all: $(PCRE) tmpfs

$(PCRE):
	mkdir -p $(BUILD)
	cd $(BUILD); \
	unset LD; \
	[ -f Makefile ] || \
          ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--enable-shared=yes \
		; \
	$(MAKE); \
	$(MAKE) install;

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib
	$(TMPFSINST) $(BUILD)/bin /bin

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
	rm $(BUILD) -rf
