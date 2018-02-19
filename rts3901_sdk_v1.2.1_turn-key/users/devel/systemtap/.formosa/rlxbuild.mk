#
# Realtek Semiconductor Corp.
#
# Jun-Ru Chang (jrjang@realtek.com)
#

BUILD:=$(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

BINARY:=dtrace stap stap-merge stap-report staprun stapsh
LIBEXEC:=$(CROSS_TARGET)-stap-env $(CROSS_TARGET)-stapio

all:
	$(Q)mkdir -p $(BUILD)
	$(Q)cd $(BUILD);    \
	ac_cv_prog_have_javac=no \
	ac_cv_prog_have_jar=no \
	LIBS="-lintl -liconv -lrt -luargp" \
	../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--disable-ssp \
		--without-nss \
		--without-avahi; \
	$(MAKE); \
	$(MAKE) install

romfs:
	$(Q)for f in $(BINARY); do \
		$(ROMFSINST) $(BUILD)/bin/$(CROSS_TARGET)-$$f /bin; \
	done
	$(Q)for f in $(LIBEXEC); do \
		$(ROMFSINST) $(BUILD)/libexec/systemtap/$$f /libexec/systemtap/$$f; \
	done

clean:
	$(Q)rm -rf $(BUILD)
