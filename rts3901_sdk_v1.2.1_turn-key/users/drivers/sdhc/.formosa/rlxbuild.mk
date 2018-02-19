#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
SCRIPT_NAME := 03_init_sdhc
MODULE_PATH := /usr/lib/modules
MIDFS_PATH := /modules
.PHONY: all bootscript midfs romfs clean

all: target bootscript midfs

target:
	$(MAKE)

bootscript:
	rm $(SCRIPT_NAME) -f
	echo "#!/bin/sh" >> $(SCRIPT_NAME)
	echo "#this is automatically generated" >> $(SCRIPT_NAME)
	echo -e "\n" >> $(SCRIPT_NAME)
	echo "insmod $(MODULE_PATH)/rtsx-icr.ko" >> $(SCRIPT_NAME)
	chmod 755 $(SCRIPT_NAME)

midfs:
	$(MIDFSINST) -S -d rtsx-icr.ko $(MIDFS_PATH)/rtsx-icr.ko

romfs:
	$(ROMFSINST) -S rtsx-icr.ko $(MODULE_PATH)
	$(ROMFSINST) $(SCRIPT_NAME) /etc/preinit/$(SCRIPT_NAME)

clean:
	rm $(SCRIPT_NAME) -f
	$(MAKE) clean

