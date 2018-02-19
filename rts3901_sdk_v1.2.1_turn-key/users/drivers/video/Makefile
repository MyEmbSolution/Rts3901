#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
KSRC := $(DIR_LINUX)
ARCH := rlx
EXTRA_CFLAGS += -Werror
MODULE_RTS_CAMERA := rts_cam
obj-m += $(MODULE_RTS_CAMERA).o
$(MODULE_RTS_CAMERA)-objs += rts_camera.o
$(MODULE_RTS_CAMERA)-objs += rts_camera_dev.o
$(MODULE_RTS_CAMERA)-objs += rts_camera_video.o
$(MODULE_RTS_CAMERA)-objs += rts_camera_ctrl.o
$(MODULE_RTS_CAMERA)-objs += rts_camera_mem.o
$(MODULE_RTS_CAMERA)-objs += rts-dma-contig.o


ifdef CONFIG_VIDEO_RTS_SOC

ifdef CONFIG_RTS_MCU_FW_CUSTOM
EXTRA_CFLAGS += -DCONFIG_RTS_MCU_FW_CUSTOM
else
EXTRA_CFLAGS += -DCONFIG_RTS_MCU_FW_IN_DDR
endif

obj-m += rts_camera_soc.o
endif

ifdef CONFIG_VIDEO_HX280_ENC
MODULE_VIDEO_HX280_ENC := rts_camera_hx280enc
obj-m += $(MODULE_VIDEO_HX280_ENC).o
endif

ifdef CONFIG_RTS_MJPEG_ENC
MODULE_RTS_MJPEG_ENC := rts_camera_jpgenc
obj-m += $(MODULE_RTS_MJPEG_ENC).o
endif

ifdef CONFIG_RTS_RTSTREAM_INFO
MODULE_RTS_RTSTREAM_INFO := rtstream
obj-m += $(MODULE_RTS_RTSTREAM_INFO).o
endif

.PHONY: all clean

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd) modules \

clean:
	rm -f *.o
	rm -f *.mod.c *.mod.o
	rm -f modules.order
	rm -f *.ko
	rm -f Module.symvers

