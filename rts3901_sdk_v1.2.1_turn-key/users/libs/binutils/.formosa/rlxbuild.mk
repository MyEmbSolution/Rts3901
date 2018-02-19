#
# Realtek Semiconductor Corp.
#
# Viller Hsiao (villerhsiao@realtek.com)
# Mar. 02, 2010
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)

BUILD := $(shell pwd)/.formosa/build

.PHONY: all romfs tmpfs clean
.PHONY: bfd libiberty opcodes binutils

all: prefs binutils tmpfs

prefs:
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/include
	mkdir -p $(BUILD)/lib

binutils: libiberty bfd opcodes
	mkdir -p $(BUILD)/binutils; \
	cd $(BUILD)/binutils; \
	[ -f Makefile ] || \
	  ../../../binutils/configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--disable-werror \
		--disable-shared; \
	$(MAKE)

bfd:
	mkdir -p $(BUILD)/bfd; \
	cd $(BUILD)/bfd; \
	[ -f Makefile ] || \
	  ../../../bfd/configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--disable-werror \
		--disable-shared; \
	$(MAKE); \
	$(MAKE) install

libiberty:
	mkdir -p $(BUILD)/libiberty; \
	cd $(BUILD)/libiberty; \
	[ -f Makefile ] || \
	  ../../../libiberty/configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--disable-shared; \
	$(MAKE); \
	cp libiberty.a $(BUILD)/lib

opcodes:
	mkdir -p $(BUILD)/opcodes; \
	cd $(BUILD)/opcodes; \
	[ -f Makefile ] || \
	  ../../../opcodes/configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--disable-shared; \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	for x in $(shell ls $(BUILD)/include); \
		do $(TMPFSINST) $(BUILD)/include/$$x /include/$$x; \
	done
	for x in $(shell ls $(BUILD)/lib); \
		do $(TMPFSINST) $(BUILD)/lib/$$x /lib/$$x; \
	done

romfs:
	$(ROMFSINST) $(BUILD)/binutils/objdump /bin/objdump

clean:	
	rm -rf $(BUILD) 
