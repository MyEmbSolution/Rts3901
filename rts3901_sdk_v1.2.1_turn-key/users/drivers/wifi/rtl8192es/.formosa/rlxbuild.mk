#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
SCRIPT_PREFIX := 03_init_
MODULE_PATH := /usr/lib/modules
ADAPTER_NAME := $(notdir $(shell pwd))
SCRIPT_NAME := $(SCRIPT_PREFIX)$(ADAPTER_NAME)
MIDFS_PATH := /modules

.PHONY: all target bootscript midfs romfs clean

all: target bootscript midfs

target:
	$(MAKE)

bootscript:
	rm $(SCRIPT_NAME) -f
	echo "#!/bin/sh" >> $(SCRIPT_NAME)
	echo "#this is automatically generated" >> $(SCRIPT_NAME)
	echo -e "\n" >> $(SCRIPT_NAME)
	echo "insmod $(MODULE_PATH)/8192es.ko" >> $(SCRIPT_NAME)
	chmod 755 $(SCRIPT_NAME)

midfs:
	$(MIDFSINST) -S -d 8192es.ko $(MIDFS_PATH)/8192es.ko

romfs:
	$(ROMFSINST) -S 8192es.ko $(MODULE_PATH)
	$(ROMFSINST) $(SCRIPT_NAME) /etc/preinit/$(SCRIPT_NAME)

clean:
	rm $(SCRIPT_NAME) -f
	$(MAKE) clean

