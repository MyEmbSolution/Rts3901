#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
SCRIPT_NAME := 03_init_audio
MODULE_PATH := /usr/lib/modules
MIDFS_PATH := /modules
.PHONY: all target romfs midfs clean

all: target bootscript midfs

target:
	$(MAKE)

bootscript:
	rm $(SCRIPT_NAME) -f
	echo "#!/bin/sh" >> $(SCRIPT_NAME)
	echo "#this is automatically generated" >> $(SCRIPT_NAME)
	echo "#please ensure rlx_snd.ko is inserted at last" >> $(SCRIPT_NAME)
	echo -e "\n" >> $(SCRIPT_NAME)
	echo "insmod $(MODULE_PATH)/rlx_dma.ko" >> $(SCRIPT_NAME)
	echo "insmod $(MODULE_PATH)/rlx_i2s.ko" >> $(SCRIPT_NAME)
ifneq ($(CONFIG_SND_SOC_RLX_INTERN_CODEC),)
	echo "insmod $(MODULE_PATH)/rlx_codec.ko" >> $(SCRIPT_NAME)
endif
	echo "insmod $(MODULE_PATH)/rlx_snd.ko" >> $(SCRIPT_NAME)
	chmod 755 $(SCRIPT_NAME)

midfs:
	$(MIDFSINST) -S -d rlx_dma.ko $(MIDFS_PATH)/rlx_dma.ko
	$(MIDFSINST) -S -d rlx_i2s.ko $(MIDFS_PATH)/rlx_i2s.ko
ifneq ($(CONFIG_SND_SOC_RLX_INTERN_CODEC),)
	$(MIDFSINST) -S -d rlx_codec.ko $(MIDFS_PATH)/rlx_codec.ko
endif
	$(MIDFSINST) -S -d rlx_snd.ko $(MIDFS_PATH)/rlx_snd.ko

romfs:
	$(ROMFSINST) -S rlx_dma.ko $(MODULE_PATH)
	$(ROMFSINST) -S rlx_i2s.ko $(MODULE_PATH)
ifneq ($(CONFIG_SND_SOC_RLX_INTERN_CODEC),)
	$(ROMFSINST) -S rlx_codec.ko $(MODULE_PATH)
endif
	$(ROMFSINST) -S rlx_snd.ko $(MODULE_PATH)
	$(ROMFSINST) $(SCRIPT_NAME) /etc/preinit/$(SCRIPT_NAME)

clean:
	rm $(SCRIPT_NAME) -f
	$(MAKE) clean

