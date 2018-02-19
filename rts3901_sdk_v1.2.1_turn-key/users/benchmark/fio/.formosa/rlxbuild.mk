#
# Realtek Semiconductor Corp.
#
# Jun-Ru Chang (jrjang@realtek.com)
# Feb. 6, 2014

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: apps romfs clean
all: apps

apps:
	./configure \
		--cc=$(CROSS_COMPILE)gcc \
		--extra-cflags="$(CFLAGS)"; \
	$(MAKE)

romfs:
	$(Q)$(ROMFSINST) fio /bin

clean:
	if [ -f config-host.mak ]; then \
		$(MAKE) distclean; \
	fi
