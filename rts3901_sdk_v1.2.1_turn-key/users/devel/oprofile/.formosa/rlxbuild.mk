#
# Realtek Semiconductor Corp.
#
# Viller Hsiao (villerhsiao@realtek.com)
# Mar. 02, 2010
#

DIR_ROOT := $(shell pwd)
DIR_OPROFILE := $(DIR_ROOT)
DIR_BUILD := $(DIR_ROOT)/.formosa/build

.PHONY: all romfs clean

all:
	$(Q)[ -d $(DIR_BUILD)/oprofile ] || mkdir -p $(DIR_BUILD)/oprofile; \
	cd $(DIR_BUILD)/oprofile; \
	[ -f Makefile ] || \
	CFLAGS="-I$(DIR_TMPFS)/include $(CFLAGS)" \
	CXXFLAGS="-I$(DIR_TMPFS)/include $(CXXFLAGS)" \
	LDFLAGS="-L$(DIR_TMPFS)/lib $(LDFLAGS)" \
	enable_gui=no \
	$(DIR_OPROFILE)/configure \
		--prefix=$(DIR_BUILD) \
		--host=$(CROSS_TARGET) \
		--disable-shared \
		--enable-gui=no \
		--with-kernel=$(DIR_LINUX)/include \
		--with-extra-libs=$(DIR_TMPFS)/lib \
		--with-extra-includes=$(DIR_TMPFS)/include; \
	$(MAKE) install

romfs:
	$(Q)for x in $(shell ls $(DIR_BUILD)/bin); do \
		$(ROMFSINST) $(DIR_BUILD)/bin/$$x /bin/$$x; \
	done
	$(Q)$(ROMFSINST) -d $(DIR_BUILD)/share/oprofile /usr/share/oprofile
	$(Q)$(ROMFSINST) $(shell pwd)/mtab /etc/mtab
	$(Q)$(ROMFSINST) $(shell pwd)/S30init_oprofile /etc/rcS.d

clean:
	$(Q)rm -rf $(DIR_BUILD)
