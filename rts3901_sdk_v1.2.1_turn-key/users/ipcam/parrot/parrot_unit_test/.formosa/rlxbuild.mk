BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all: prepare parrot_cunit_build

prepare:
	mkdir -p $(BUILD)
	cp ../parrot.c ut_parrot.c
	cp ../video.c video.c
	sed -i "s/int main/int __main/g" ut_parrot.c
	cat test_parrot.c >> ut_parrot.c

parrot_cunit_build:
	cd $(BUILD); \
	[ -f Makefile ] || \
	  cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake; \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(ROMFSINST) $(BUILD)/bin/parrot_cunit /bin

clean:
	rm -rf $(BUILD) ut_parrot.c video.c
