#
# Realtek Semiconductor Corp.
#
# Tony Nie (tony_nie@realsil.com.cn)
# Aug. 14, 2014
#

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

LIGHTTPD = lighttpd
TARGET = lighttpd_target

STARTUP_SCRIPT = S91lighttpd

DIR_SOURCE := $(shell pwd)
DIR_OUTPUT := $(shell pwd)/.formosa/build
DIR_CONF := $(shell pwd)/rs_conf
AVAILABLE_LOC:= $(shell pwd)/rs_conf/conf-available
AVAILABLE_DST :=/etc/$(LIGHTTPD)/conf-available
DIR_LIBS_DST :=/usr/lib/$(LIGHTTPD)

all: $(TARGET)

$(TARGET):
	mkdir -p $(DIR_OUTPUT)
	cd $(DIR_SOURCE); \
	unset LD; \
	export PCRECONFIG="$(DIR_TMPFS)/bin/pcre-config" ; \
	[ -e configure ] || ./autogen.sh; \
	[ -e Makefile ] || ./configure \
		--prefix=$(DIR_OUTPUT) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--without-valgrind \
		--with-openssl \
		--without-kerberos5 \
		--without-bzip2 \
		--without-zlib \
		--without-lua \
		--with-pcre \
		--with-auth \
		--without-webdav-props \
		--disable-lfs \
		; \
	$(MAKE) ; \
	$(MAKE) install;

romfs:
	mkdir -p $(DIR_ROMFS)/$(DIR_LIBS_DST)
	for x in $(shell cd $(DIR_OUTPUT)/lib; ls *.so*); do \
		if [ -L $(DIR_OUTPUT)/lib/$$x ]; then \
			dest=`readlink $(DIR_OUTPUT)/lib/$$x`; \
			$(ROMFSINST) -s $$dest $(DIR_LIBS_DST)/$$x; \
		else \
			$(ROMFSINST) $(DIR_OUTPUT)/lib/$$x $(DIR_LIBS_DST)/ ; \
		fi; \
	done
	$(ROMFSINST) $(DIR_OUTPUT)/sbin/mips-linux-lighttpd /bin
	$(ROMFSINST) $(STARTUP_SCRIPT) /etc/rcS.d
	mkdir -p $(DIR_ROMFS)/etc/conf
	$(ROMFSINST) $(DIR_CONF)/htdigest_admin.txt /etc/conf/
	$(ROMFSINST) $(DIR_CONF)/htdigest_user.txt /etc/conf/
	mkdir -p $(DIR_ROMFS)/etc/$(LIGHTTPD)
	$(ROMFSINST) $(DIR_CONF)/lighttpd.conf /etc/lighttpd/
	mkdir -p $(DIR_ROMFS)/$(AVAILABLE_DST)
	for x in $(shell ls $(AVAILABLE_LOC)/*.conf); do \
		if [ -L $(AVAILABLE_LOC)/$$x ]; then \
			dest=`readlink $$x`; \
			$(ROMFSINST) -s $$dest $(AVAILABLE_DST)/$$x; \
		else \
			$(ROMFSINST) $$x $(AVAILABLE_DST)/ ; \
		fi; \
	done

clean:
	-rm $(DIR_OUTPUT) -rf
	if [ -e $(DIR_SOURCE)/Makefile ]; then \
		$(MAKE) -C $(DIR_SOURCE) clean; \
	fi
	-rm -f $(DIR_SOURCE)/configure
	-rm -f $(DIR_SOURCE)/Makefile
