#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all: libs tmpfs

libs:
	CC=$(CROSS_COMPILE)gcc \
	AR=$(CROSS_COMPILE)ar \
	RANLIB=$(CROSS_COMPILE)ranlib \
	$(MAKE)

tmpfs:
	$(MAKE) install prefix=$(DIR_TMPFS)

romfs:
	for x in $(shell cd $(DIR_TMPFS)/lib; ls libaio*.so*); do \
		if [ -L $(DIR_TMPFS)/lib/$$x ]; then \
			dest=`readlink $(DIR_TMPFS)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(DIR_TMPFS)/lib/$$x /lib; \
		fi; \
	done

clean:
	$(MAKE) clean
