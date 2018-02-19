#
# Realtek Semiconductor Corp.
#
# Viller Hsiao (villerhsiao@realtek.com)
# Mar. 02, 2010
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

CFLAGS ?= -Os -I$(DIR_TMPFS)/include
LDFLAGS ?= -L$(DIR_TMPFS)/lib

BUILD := $(shell pwd)/.formosa/build

all:
	mkdir -p $(BUILD);
	cd $(BUILD); \
	[ -f Makefile ] || \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)" \
	../../configure \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET); \
	$(MAKE)

romfs:
	$(ROMFSINST) $(BUILD)/strace /bin/strace

clean:
	rm -rf $(BUILD)
