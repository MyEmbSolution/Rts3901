############################################################################
#
# Realtek Semiconductor Corp.
#
# Makefile -- Top level dist makefile.
#
# Tony Wu (tonywu@realtek.com)
# Oct. 12, 2008
#

DIR_ROOT = $(shell pwd)
VAR_HOST = $(shell uname | tr '[:upper:]' '[:lower:]')
export DIR_ROOT VAR_HOST

PREWORK := mtdmapping
MTDPART = $(DIR_ROOT)/utils/scripts/mtdpart.py

ifneq (.config,$(wildcard .config))
TARGET := config
else

include .config

#
# Variables
#
DIR_RSDK = $(CONFIG_RSDKDIR)

ifeq ($(CONFIG_ARCH_CPU_ARM64),y)
VAR_ARCH:= arm64
CROSS_TARGET := arm-linux
CROSS_COMPILE:= asdk64-linux-
endif
ifeq ($(CONFIG_ARCH_CPU_ARM),y)
VAR_ARCH:= arm
CROSS_TARGET := arm-linux
CROSS_COMPILE:= asdk-linux-
endif
ifeq ($(CONFIG_ARCH_CPU_RLX),y)
VAR_ARCH:= rlx
CROSS_TARGET := mips-linux
CROSS_COMPILE:= rsdk-linux-
endif
ifeq ($(CONFIG_ARCH_CPU_MIPS),y)
VAR_ARCH:= mips
CROSS_TARGET := mips-linux
CROSS_COMPILE:= msdk-linux-
endif

ifeq ($(CONFIG_ARCH_CPU_EL),y)
VAR_ENDIAN := EL
else
VAR_ENDIAN := EB
endif

#
# Directories
#
DIR_BOARD = $(DIR_ROOT)/target
DIR_ROMFS = $(DIR_ROOT)/target/romfs
DIR_TMPFS = $(DIR_ROOT)/target/tmpfs
DIR_MIDFS = $(DIR_ROOT)/target/midfs
DIR_EXTFS = $(DIR_ROOT)/target/extfs
DIR_IMAGE = $(DIR_ROOT)/target/image
DIR_USERS = $(DIR_ROOT)/users
DIR_LINUX = $(DIR_ROOT)/$(CONFIG_LINUXDIR)
DIR_BTLOAD = $(DIR_ROOT)/bootloader
DIR_BSP   = $(DIR_ROOT)/target/bsp

ROMFSINST = $(DIR_ROOT)/config/romfsinst
TMPFSINST = $(DIR_ROOT)/config/tmpfsinst
MIDFSINST = $(DIR_ROOT)/config/midfsinst
EXTFSINST = $(DIR_ROOT)/config/extfsinst
WRAPPER :=  $(DIR_RSDK)/bin/$(CROSS_COMPILE)wrapper

MAKE = PATH=$(DIR_RSDK)/bin:$(DIR_ROOT)/config/$(VAR_HOST):$(PATH); make --no-print-directory
SSTRIP = PATH=$(DIR_RSDK)/bin:$(PATH); $(CROSS_COMPILE)sstrip
LSTRIP = PATH=$(DIR_RSDK)/bin:$(PATH); $(CROSS_COMPILE)lstrip
ifeq ($(CONFIG_OPT_KEEP),y)
LSTRIP += -keep
endif
ifeq ($(CONFIG_OPT_NO_LSTRIP),y)
LSTRIP += -nostrip
endif
ifeq ($(CONFIG_OPT_NO_LSHRINK),y)
LSTRIP += -noshrink
endif
ifeq ($(CONFIG_OPT_DEBUG_LSHRINK),y)
LSTRIP += -debug
endif

#
# Optimization
#
CROSS_CFLAGS ?= $(CONFIG_CCFLAGS)

export VAR_ARCH CROSS_COMPILE CROSS_TARGET CROSS_CFLAGS
export DIR_RSDK DIR_ROMFS DIR_IMAGE DIR_BOARD DIR_TMPFS DIR_MIDFS DIR_EXTFS
export DIR_LINUX ROMFSINST TMPFSINST MIDFSINST EXTFSINST

ERROR :=
ifneq ($(CONFIG_LINUXDIR),$(wildcard $(CONFIG_LINUXDIR)))
ERROR += '$(CONFIG_LINUXDIR) does not exist '
endif

ifneq ($(CONFIG_RSDKDIR),$(wildcard $(CONFIG_RSDKDIR)))
ERROR += '$(CONFIG_RSDKDIR) does not exist'
endif

ifeq ($(ERROR),)
TARGET := linux bins romfs image
else
TARGET := error
endif

ifneq ($(CONFIG_BTLOAD_NONE),y)
TARGET += bootloader
endif
endif

ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
  endif
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif
export Q

ifdef R
  ifeq ("$(origin R)", "command line")
    ADB_PORT = 7788
  endif
endif
ifndef ADB_PORT
  ADB_PORT = 5555
endif

ifdef CPU
  ifeq ("$(origin CPU)", "command line")
    SMP = $(CPU)
  endif
endif
ifndef SMP
  SMP = 1
endif

ifdef GPU
  ifeq ("$(origin GPU)", "command line")
    GPU_NAME = $(GPU)
  endif
endif

LINUX_CFG=$(wildcard $(DIR_LINUX)/.config)
ifeq (.config,$(notdir $(LINUX_CFG)))
include $(DIR_LINUX)/.config
endif

all: $(PREWORK) $(TARGET)

error:
	$(Q)echo
	$(Q)echo "================================= NOTICE ================================="
	$(Q)echo
	$(Q)for X in $(ERROR) ; do \
		echo ERROR: $$X; \
	done
	$(Q)echo
	$(Q)echo "Please run 'make config' to reconfigure"
	$(Q)echo

mtdmapping:
	$(Q)rm -f $(DIR_BSP)/partition.ini
ifeq ($(CONFIG_ROOTFS_RAMFS),y)
	$(Q)ln -s partition_ramfs.ini $(DIR_BSP)/partition.ini
else
	$(Q)ln -s partition_squashfs.ini $(DIR_BSP)/partition.ini
endif
	$(MTDPART) $(DIR_ROOT)

#
# 1. target configuration
#
.PHONY: config menuconfig
config menuconfig:
	$(Q)chmod u+x config/genconfig
	$(Q)chmod u+x config/setconfig
	$(Q)if [ ! -d $(DIR_RSDK) ]; then \
		echo; \
		echo "ERROR: no toolchain found"; \
		echo; \
		echo "Please place wrapper under toolchain/ directory"; \
		exit 1; \
	fi
	$(Q)if [ ! -e config/$(VAR_HOST)/mconf ]; then \
		echo; \
		echo "ERROR: host $(VAR_HOST) not supported yet"; \
		echo; \
		echo "Please contact processor team"; \
		exit 1; \
	fi
	$(Q)config/genconfig > Kconfig
	$(Q)config/$(VAR_HOST)/mconf Kconfig

#
# 2.0 kernel image
#
.PHONY: linux
linux:
	$(Q)$(MAKE) -C $(DIR_LINUX) || exit $$?

#
# 2.1 user app
#
.PHONY: users bins
users bins:
	$(Q)config/hdrconfig $(DIR_ROOT)
	$(Q)$(MAKE) -C $(DIR_USERS) || exit $$?

#
# 3. romfs
#
.PHONY: romfs
romfs:
	$(Q)rm -rf $(DIR_ROMFS)/*
	$(Q)rm -rf $(DIR_EXTFS)/*
	$(Q)$(MAKE) -C $(DIR_BOARD) romfs || exit $$?
	$(Q)cp -R $(DIR_RSDK)/lib/*.so $(DIR_ROMFS)/lib
	$(Q)cp -R $(DIR_RSDK)/lib/*.so.* $(DIR_ROMFS)/lib
	$(Q)chmod 755 $(DIR_ROMFS)/lib/*.so
	$(Q)$(MAKE) -C $(DIR_USERS) romfs || exit $$?
	$(Q)$(LSTRIP) $(DIR_ROMFS) $(DIR_EXTFS)
	$(Q)$(MAKE) -C $(DIR_LINUX) modules_install \
		INSTALL_MOD_PATH=$(DIR_ROMFS) || exit $$?

#
# 4. image and bootloader
#
.PHONY: image
image:
	$(Q)rm -f $(DIR_IMAGE)/*
	$(Q)echo "INFO: create kernel images"
	$(Q)touch $(DIR_LINUX)/usr/gen_init_cpio
	$(Q)$(MAKE) -C $(DIR_LINUX) || exit $$?
	$(Q)echo "INFO: create rootfs images"
	$(Q)$(MAKE) -C $(DIR_BOARD) image

.PHONY: bootloader
bootloader:
	$(Q)$(MAKE) -C $(DIR_BTLOAD)

#
# 5. pack linux image and bootloader
#
.PHONY: pack
pack:
	$(Q)$(MAKE) -C $(DIR_BOARD) pack

.PHONY: test
test:
	$(Q)(cd emulator; ./qemu.sh -$(VAR_ARCH) -$(VAR_ENDIAN) -port=$(ADB_PORT) -smp=$(SMP) -gpu=$(GPU_NAME))

#
# x. short-cut to build and clean one application
#
linux_clean:
	$(Q)$(MAKE) -C $(DIR_LINUX) -s clean

bootloader_clean:
	$(Q)$(MAKE) -C $(DIR_BTLOAD) -s clean

%_build:
	$(Q)$(MAKE) -C users $(@:_build=)

%_clean %_romfs %_tmpfs:
	$(Q)$(MAKE) -C users $@

#
# z. wrapper update
#

.PHONY: wrapper
wrapper:
	$(Q)echo "WRAPPER rebuilding wrapper ..."
	$(Q)echo "WRAPPER updating headers from $(DIR_LINUX)"
	$(Q)if $(WRAPPER) -kernel $(DIR_LINUX) -silent; then \
		echo "WRAPPER wrapper update completed"; \
	fi

.PHONY: clean
clean:
	$(Q)echo "CLEAN $(DIR_USERS)"
	$(Q)$(MAKE) -C $(DIR_USERS) -s clean
	$(Q)echo "CLEAN $(DIR_LINUX)"
	$(Q)$(MAKE) -C $(DIR_LINUX) -s clean
	$(Q)if [ -d $(DIR_BTLOAD) ]; then \
		echo "CLEAN $(DIR_BTLOAD)"; \
		$(MAKE) -C $(DIR_BTLOAD) -s clean; \
	fi
	$(Q)rm -rf $(DIR_ROMFS)/*
	$(Q)rm -rf $(DIR_TMPFS)/*
	$(Q)rm -rf $(DIR_MIDFS)/*
	$(Q)rm -rf $(DIR_EXTFS)/*

.PHONY: mrproper
mrproper:
	$(Q)echo "CLEAN config files"
	$(Q)$(MAKE) -C $(DIR_LINUX) -s mrproper
	$(Q)rm -rf .config .config.old .oldconfig Kconfig
	$(Q)rm -f image romfs extfs tmpfs
	$(Q)rm -f target/image target/romfs target/extfs target/tmpfs
	$(Q)rm -f target/selection
	$(Q)rm -rf target/box
	$(Q)rm -f target
