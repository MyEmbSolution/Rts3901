#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)

BUILD := $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	    LIBS="-liconv -lrt -lpthread $(LIBS)" \
	    ../../configure \
		--prefix=$(DIR_TMPFS) \
		--host=$(CROSS_TARGET) \
		--disable-libasprintf \
		--disable-acl \
		--disable-openmp \
		--disable-rpath \
		--disable-java \
		--disable-native-java \
		--disable-csharp \
		--disable-relocatable \
		--disable-tools \
		--without-emacs; \
	$(MAKE); \
	$(MAKE) install

romfs:
	for x in $(shell cd $(DIR_TMPFS)/lib; ls libintl*.so*); do \
		if [ -L $(DIR_TMPFS)/lib/$$x ]; then \
			dest=`readlink $(DIR_TMPFS)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(DIR_TMPFS)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	$(Q)rm -rf $(BUILD)
