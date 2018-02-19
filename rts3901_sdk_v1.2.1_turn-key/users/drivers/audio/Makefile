#
# Realtek Semiconductor Corp.
#
# Jim Cao (jim_cao@realsil.com.cn)
# July. 22, 2016
#
KSRC := $(DIR_LINUX)
ARCH := rlx
EXTRA_CFLAGS += -Werror

obj-m += rlx_dma.o
obj-m += rlx_i2s.o
obj-m += rlx_snd.o

ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC
EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_INTERN_CODEC
obj-m += rlx_codec.o
ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC_LINEIN
EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_INTERN_CODEC_LINEIN
endif
ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC_AMIC
EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_INTERN_CODEC_AMIC
ifdef CONFIG_SND_SOC_RLX_AMIC_IN_LDO
EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_AMIC_IN_LDO
endif
ifdef CONFIG_SND_SOC_RLX_AMIC_PMU_RTP
EXTRA_CFLAGS += -DSND_SOC_RLX_AMIC_PMU_RTP
ifdef CONFIG_SND_SOC_RLX_AMIC_PMU_RTP_LDO1
EXTRA_CFLAGS += -DSND_SOC_RLX_AMIC_PMU_RTP_LDO1
endif
endif
endif
endif

#TODO: external card codec driver is needed to move
#      as kernel modules to support external card config
#ifdef CONFIG_SND_SOC_RLX_EXTERN_CODEC
#EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_EXTERN_CODEC
#ifdef CONFIG_SND_SOC_RLX_RT5651
#EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_RT5651
#endif
#endif

ifdef CONFIG_SND_SOC_RLX_DEBUG
EXTRA_CFLAGS += -DCONFIG_SND_SOC_RLX_DEBUG
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

