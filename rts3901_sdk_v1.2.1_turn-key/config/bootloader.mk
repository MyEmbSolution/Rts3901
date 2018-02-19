# Realtek Semiconductor Corp.
#
# Jethro Hsu (jethro@realtek.com)
# Jun. 28, 2012
#

DIR_ROOT ?= $(shell pwd)
DIR_IMAGE ?= $(DIR_ROOT)/image
DIR_TMPFS ?= $(DIR_ROOT)/tmpfs
DIR_ROMFS ?= $(DIR_ROOT)/romfs
DIR_EXTFS ?= $(DIR_ROOT)/extfs
ROMFSINST ?= $(DIR_ROOT)/config/romfsinst
TMPFSINST ?= $(DIR_ROOT)/config/tmpfsinst

.EXPORT_ALL_VARIABLES:
.PHONY: all bootloader clean

CROSS_COMPILE ?= mips-linux-
CROSS_TARGET ?= mips-linux
CROSS_CFLAGS ?= -Os -fstrict-aliasing -fstrict-overflow
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP = $(CROSS_COMPILE)strip
SSTRIP = $(CROSS_COMPILE)sstrip

CFLAGS := $(CROSS_CFLAGS) -I$(DIR_TMPFS)/include
CXXFLAGS := $(CROSS_CFLAGS) -I$(DIR_TMPFS)/include
LDFLAGS := -L$(DIR_TMPFS)/lib
LDLIBS :=

-include ../.config

ERROR :=
ifneq (../.config,$(wildcard ../.config))
  ERROR += '../.config does not exist '
endif

TARGET :=
BTLDIR := $(subst bootloader/,,$(CONFIG_BTLOADDIR))
ifneq ($(BTLDIR), NONE)
  ifneq ($(BTLDIR),$(wildcard $(BTLDIR)))
    ERROR += '$(BTLDIR) does not exist'
  endif

  ifeq ($(ERROR),)
    TARGET = bootloader
  else
    TARGET = error
  endif
endif

all: $(TARGET)

bootloader:
	$(Q)$(MAKE) -C $(BTLDIR) -f rlxbuild.mk

error:
	@echo
	@echo "=== Building Bootloader ==="
	@echo
	@for X in $(ERROR) ; do \
		echo ERROR: $$X; \
	done
	@echo
	@echo "Please run 'make config' to reconfigure or check your source tree"
	@echo

clean:
	@if [ x$(BTLDIR) != xNONE ]; then \
		$(MAKE) -C $(BTLDIR) -f rlxbuild.mk clean; \
	fi
