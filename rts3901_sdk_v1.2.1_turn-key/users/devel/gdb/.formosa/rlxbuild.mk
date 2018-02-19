#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

CROSS_COMPILE ?= mips-linux-
CROSS_TARGET ?= mips-linux

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
RANLIB=$(CROSS_COMPILE)ranlib
STRIP=$(CROSS_COMPILE)strip

CFLAGS := $(CROSS_CFLAGS)
LDFLAGS = -static

TARGET_CONFIG = CC=$(CROSS_COMPILE)gcc \
		CXX=$(CROSS_COMPILE)g++ \
		LD=$(CROSS_COMPILE)gcc \
		AR=$(CROSS_COMPILE)ar \
		RANLIB=$(CROSS_COMPILE)ranlib

BUILD := $(shell pwd)/.formosa/build
BUILD_TERMCAP := $(BUILD)/libtermcap

all:
	mkdir -p $(BUILD_TERMCAP)
	cd $(BUILD_TERMCAP); \
	[ -f Makefile ] || \
	  $(TARGET_CONFIG) \
		../../../libtermcap/configure \
			--prefix=$(DIR_TMPFS) \
			--host=$(CROSS_TARGET) \
			--target=$(CROSS_TARGET) \
			--disable-werror; \
	  $(MAKE); \
	  $(MAKE) install
	cd $(BUILD); \
	  [ -f Makefile ] || \
		$(TARGET_CONFIG) \
		CFLAGS="-I$(DIR_TMPFS)/include $(CFLAGS)" \
		LDFLAGS="-L$(DIR_TMPFS)/lib $(LDFLAGS)" \
		LIBS="-ltermcap" \
		../../configure \
			--host=$(CROSS_TARGET) \
			--target=$(CROSS_TARGET) \
			--disable-werror; \
	  $(MAKE)

romfs:
	$(ROMFSINST) $(BUILD)/gdb/gdbserver/gdbserver /bin/gdbserver

clean:
	$(Q)rm -rf $(BUILD)
