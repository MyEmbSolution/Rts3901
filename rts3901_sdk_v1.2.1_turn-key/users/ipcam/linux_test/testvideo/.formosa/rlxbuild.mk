#
# Realtek Semiconductor Corp.
#
# Ming Qian (ming_qian@realsil.com.cn)
#
BUILD = $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all target

all: target

target:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  cmake ../.. \
		-DCMAKE_INSTALL_PREFIX=$(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(DIR_ROOT)/config/toolchain_rsdk.cmake; \
	$(MAKE); \
	$(MAKE) install

tmpfs:
	#$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/bin /usr/bin
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

