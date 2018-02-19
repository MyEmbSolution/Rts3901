#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Jan. 10, 2014

VAR_ARCH ?= mips

ifeq ($(VAR_ARCH),mips)
CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CROSS_OPENSSL := linux-mips32
endif

ifeq ($(VAR_ARCH),rlx)
CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CROSS_OPENSSL := linux-mips32
endif

ifeq ($(VAR_ARCH),arm)
CROSS_TARGET ?= arm-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CROSS_OPENSSL := linux-generic32
endif

.PHONY: libs tmpfs romfs clean
all: libs tmpfs

libs:
	[ -f Makefile ] || \
	./Configure \
		$(CROSS_OPENSSL) \
		threads \
		shared \
		--prefix=$(DIR_TMPFS) \
		--openssldir=$(DIR_TMPFS)/etc/ssl \
		--with-zlib-lib=$(DIR_TMPFS) 2>&1 > /dev/null;\
	$(MAKE)

tmpfs:
	$(MAKE) install_sw

romfs:
	$(ROMFSINST) $(DIR_TMPFS)/etc/ssl /etc/ssl
	for x in $(shell cd $(DIR_TMPFS)/lib; ls libssl*.so* libcrypto*.so*); do \
		if [ -L $(DIR_TMPFS)/lib/$$x ]; then \
			dest=`readlink $(DIR_TMPFS)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(DIR_TMPFS)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	if [ -f Makefile ]; then \
		$(MAKE) clean; \
		rm -f Makefile; \
	fi
