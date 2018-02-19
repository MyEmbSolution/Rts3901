#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Aug. 21, 2014
#

BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all romfs clean

all:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	CFLAGS="-I$(DIR_TMPFS)/include $(CFLAGS)" \
	CXXFLAGS="-I$(DIR_TMPFS)/include $(CXXFLAGS)" \
	LDFLAGS="-L$(DIR_TMPFS)/lib $(LDFLAGS)" \
	  ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--disable-alsamixer \
		--disable-maintainer-mode \
		--disable-nls \
		--disable-xmlto \
		--with-alsa-prefix=$(DIR_TMPFS)/lib \
		--with-alsa-inc-prefix=$(DIR_TMPFS)/include; \
	$(MAKE); \

romfs:
	$(Q)for x in $(shell ls $(BUILD)/bin); do \
		$(ROMFSINST) $(BUILD)/bin/$$x /bin/$$x; \
	done
	$(ROMFSINST) $(BUILD)/amixer/amixer /bin/
	$(ROMFSINST) $(BUILD)/aplay/aplay /bin/
	$(ROMFSINST) $(BUILD)/aplay/aplay /bin/arecord

clean:
	rm -rf $(BUILD)
