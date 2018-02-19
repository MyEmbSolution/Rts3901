#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

BUILD := $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all:
	mkdir -p $(BUILD);
	cd $(BUILD); \
	[ -f Makefile ] || \
	    LIBS="-lintl -liconv -lelf" \
	    ../../configure \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--disable-werror; \
	$(MAKE)

romfs:
	$(Q)$(ROMFSINST) $(BUILD)/ltrace /bin/ltrace

clean:
	$(Q)rm -rf $(BUILD)
