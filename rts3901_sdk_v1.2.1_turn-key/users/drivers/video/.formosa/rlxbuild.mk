#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
SCRIPT_NAME := 03_init_video
MODULE_PATH := /usr/lib/modules
MIDFS_PATH := /modules

.PHONY: all target bootscript midfs tmpfs romfs clean

all: target bootscript midfs tmpfs

target:
	$(MAKE)

bootscript:
	rm $(SCRIPT_NAME) -f
	echo "#!/bin/sh" >> $(SCRIPT_NAME)
	echo "#this is automatically generated" >> $(SCRIPT_NAME)
	echo "#please ensure rts_cam.ko is inserted at first" >> $(SCRIPT_NAME)
	echo -e "\n" >> $(SCRIPT_NAME)
	echo "insmod $(MODULE_PATH)/rts_cam.ko" >> $(SCRIPT_NAME)
ifneq ($(CONFIG_VIDEO_RTS_SOC),)
	echo "insmod $(MODULE_PATH)/rts_camera_soc.ko" >> $(SCRIPT_NAME)
endif
ifneq ($(CONFIG_VIDEO_HX280_ENC),)
	echo "insmod $(MODULE_PATH)/rts_camera_hx280enc.ko" >> $(SCRIPT_NAME)
endif
ifneq ($(CONFIG_RTS_MJPEG_ENC),)
	echo "insmod $(MODULE_PATH)/rts_camera_jpgenc.ko" >> $(SCRIPT_NAME)
endif
ifneq ($(CONFIG_RTS_RTSTREAM_INFO),)
	echo "insmod $(MODULE_PATH)/rtstream.ko" >> $(SCRIPT_NAME)
endif
	chmod 755 $(SCRIPT_NAME)

midfs:
	$(MIDFSINST) -S -d rts_cam.ko $(MIDFS_PATH)/rts_cam.ko
ifneq ($(CONFIG_VIDEO_RTS_SOC),)
	$(MIDFSINST) -S -d rts_camera_soc.ko $(MIDFS_PATH)/rts_camera_soc.ko
endif
ifneq ($(CONFIG_VIDEO_HX280_ENC),)
	$(MIDFSINST) -S -d rts_camera_hx280enc.ko $(MIDFS_PATH)/rts_camera_hx280enc.ko
endif
ifneq ($(CONFIG_RTS_MJPEG_ENC),)
	$(MIDFSINST) -S -d rts_camera_jpgenc.ko $(MIDFS_PATH)/rts_camera_jpgenc.ko
endif
ifneq ($(CONFIG_RTS_RTSTREAM_INFO),)
	$(MIDFSINST) -S -d rtstream.ko $(MIDFS_PATH)/rtstream.ko
endif

tmpfs:
	 $(TMPFSINST) linux /include

romfs:
	$(ROMFSINST) -S rts_cam.ko $(MODULE_PATH)
ifneq ($(CONFIG_VIDEO_RTS_SOC),)
	$(ROMFSINST) -S rts_camera_soc.ko $(MODULE_PATH)
endif
ifneq ($(CONFIG_VIDEO_HX280_ENC),)
	$(ROMFSINST) -S rts_camera_hx280enc.ko $(MODULE_PATH)
endif
ifneq ($(CONFIG_RTS_MJPEG_ENC),)
	$(ROMFSINST) -S rts_camera_jpgenc.ko $(MODULE_PATH)
endif
ifneq ($(CONFIG_RTS_RTSTREAM_INFO),)
	$(ROMFSINST) -S rtstream.ko $(MODULE_PATH)
endif
	$(ROMFSINST) $(SCRIPT_NAME) /etc/preinit/$(SCRIPT_NAME)

clean:
	rm $(SCRIPT_NAME) -f
	$(MAKE) clean

