#
# Realtek Semiconductor Corp.
#
# Makefile
#	board Makefile for RLX Linux
#
# Tony Wu (tonywu@realtek.com)
# Dec. 12, 2007
#

DIRS = root bin dev etc etc/preinit etc/rcS.d etc/conf lib mnt proc usr usr/share usr/conf var sys media lib/firmware usr/lib/modules
MERGE_DIR = $(DIR_ROOT)/utils/scripts/
MERGE = merge.py

.PHONY: romfs image pack

LINUX_CFG=$(wildcard $(DIR_LINUX)/.config)
ifeq (.config,$(notdir $(LINUX_CFG)))
include $(DIR_LINUX)/.config
endif

romfs:
	$(Q)[ -d $(DIR_ROMFS) ] || mkdir -p $(DIR_ROMFS)
	$(Q)for i in $(DIRS); do \
		[ -d $(DIR_ROMFS)/$$i ] || mkdir -p $(DIR_ROMFS)/$$i; \
	done
	$(Q)$(ROMFSINST) rootfs/etc /etc
	$(Q)$(ROMFSINST) -s bin/init /init
	$(Q)$(ROMFSINST) -s /var/nm/resolv.conf /etc/resolv.conf
	$(Q)$(ROMFSINST) -s /var/nm/ppp /etc/ppp
	$(Q)$(ROMFSINST) -s /var/nm/dnrd /etc/dnrd
	$(Q)$(ROMFSINST) -s /var/nm/cron /etc/cron
	$(Q)$(ROMFSINST) -s /var/tmp /tmp
	$(Q)if [ -e fw/isp.fw ]; then \
		$(ROMFSINST) fw/isp.fw /lib/firmware/isp.fw; \
	fi
	$(Q)chmod 755 $(DIR_ROMFS)/etc/init.d/rcS
	$(Q)cat $(DIR_ROOT)/config/VERSION > $(DIR_ROMFS)/etc/version
	$(Q)echo `whoami`"@"`hostname` >> $(DIR_ROMFS)/etc/version
	$(Q)echo `date "+%Y-%m-%d %H:%M:%S"` >> $(DIR_ROMFS)/etc/version
	$(Q)if [ -e selection/Makefile ]; then \
		make -C selection romfs; \
	fi

image:
	$(Q)cp $(DIR_LINUX)/vmlinux $(DIR_IMAGE)/vmlinux.elf
	$(Q)if [ -e $(DIR_LINUX)/vmlinuz ]; then \
		cp $(DIR_LINUX)/vmlinuz $(DIR_IMAGE)/vmlinuz.elf; \
	fi
	$(Q)if [ -e selection/Makefile ]; then \
		make -C selection image; \
	fi
ifdef CONFIG_ROOTFS_SQUASH
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/mksquashfs $(DIR_ROMFS)/ $(DIR_IMAGE)/rootfs.bin -comp xz
endif

pack:
	cd $(MERGE_DIR); ./$(MERGE); cd -

clean:
