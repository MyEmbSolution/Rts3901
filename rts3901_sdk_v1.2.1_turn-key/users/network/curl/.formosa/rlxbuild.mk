#
# Realtek Semiconductor Corp.
#
# Elvis Chen (elvis_chen@realtek.com)
# JAN. 27, 2016
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

CURL = curl
TARGET = curl_target

DIR_SOURCE := $(shell pwd)
DIR_OUTPUT := $(shell pwd)/.formosa/build
DIR_LIBS_DST :=/lib

all: $(TARGET)

$(TARGET):
	mkdir -p $(DIR_OUTPUT)
	cd $(DIR_SOURCE); \
	unset LD; \
	[ -e Makefile ] || ./configure \
		--prefix=$(DIR_OUTPUT) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--without-ssl \
		--without-libidn --without-gnutls \
		--without-librtmp \
		--disable-rtsp \
		--disable-gopher \
		--disable-proxy \
		--disable-ipv6 \
		--disable-ldap --disable-ldaps \
		; \
	$(MAKE) ; \
	$(MAKE) install;

romfs:
	for x in $(shell cd $(DIR_OUTPUT)/lib; ls *.so*); do \
		if [ -L $(DIR_OUTPUT)/lib/$$x ]; then \
			dest=`readlink $(DIR_OUTPUT)/lib/$$x`; \
			$(ROMFSINST) -s $$dest $(DIR_LIBS_DST)/$$x; \
		else \
			$(ROMFSINST) $(DIR_OUTPUT)/lib/$$x $(DIR_LIBS_DST)/ ; \
		fi; \
	done
	$(ROMFSINST) $(DIR_OUTPUT)/bin/curl /bin
	$(ROMFSINST) $(DIR_OUTPUT)/bin/curl-config /bin

clean:
	-rm $(DIR_OUTPUT) -rf
	if [ -e $(DIR_SOURCE)/Makefile ]; then \
		$(MAKE) -C $(DIR_SOURCE) clean; \
	fi
	-rm -f $(DIR_SOURCE)/Makefile
