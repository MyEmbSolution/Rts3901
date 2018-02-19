#
# Realtek Semiconductor Corp.
#
# Jethro Hsu (jethro@realtek.com)
# Jun. 28, 2012

include ./.config
.PHONY: all clean

all:include/config/auto.conf
	$(Q)mkdir -p include/generated
	$(Q)mkdir -p include/config
	$(Q)$(MAKE) silentoldconfig
ifdef CONFIG_PARTITION_IN_BSP
	$(Q)./genconf.py $(DIR_ROOT)
else
	$(Q)./genconf.py
endif
	$(Q)if [ ! -f include/config.mk ]; then \
		$(MAKE) reconfig; \
	fi
	$(Q)$(MAKE) all
	$(Q)cp u-boot.bin $(DIR_IMAGE)/

-include include/config/auto.conf.cmd
include/config/auto.conf.cmd: ;
include/config/auto.conf: include/config/auto.conf.cmd;

clean:
	$(Q)$(MAKE) tidy
	$(Q)rm -f include/config.mk
	$(Q)rm -rf include/generated
	$(Q)rm -rf include/config
	$(Q)rm -f board/rlxboard/.depend*
