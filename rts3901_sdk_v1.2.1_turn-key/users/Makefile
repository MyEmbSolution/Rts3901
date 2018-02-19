#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Dec. 07, 2008
#

DIR_ROOT ?= $(shell pwd)
DIR_TMPFS ?= $(DIR_ROOT)/tmpfs
DIR_MIDFS ?= $(DIR_ROOT)/midfs
DIR_ROMFS ?= $(DIR_ROOT)/romfs
DIR_EXTFS ?= $(DIR_ROOT)/extfs
ROMFSINST ?= $(DIR_ROOT)/config/romfsinst
TMPFSINST ?= $(DIR_ROOT)/config/tmpfsinst
MIDFSINST ?= $(DIR_ROOT)/config/midfsinst

.EXPORT_ALL_VARIABLES:
.PHONY: all romfs clean prune

ifneq (.config,$(wildcard .config))
all: config
else
include .config
all: target
endif

CROSS_COMPILE ?= mips-linux-
CROSS_TARGET ?= mips-linux
CROSS_CFLAGS ?= -Os -fstrict-aliasing -fstrict-overflow
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP = $(CROSS_COMPILE)strip
SSTRIP = $(CROSS_COMPILE)sstrip

CFLAGS := $(CROSS_CFLAGS) -I$(DIR_TMPFS)/include
CXXFLAGS := $(CROSS_CFLAGS) -I$(DIR_TMPFS)/include
LDFLAGS := -L$(DIR_TMPFS)/lib
LDLIBS :=

pkg_y =
pkg_m =
pkg_n =
pkg_  =

include $(addsuffix /rlxbuild.in, $(shell find . -name .formosa))

FORMOSA_MK := .formosa/rlxbuild.mk

target: $(notdir $(pkg_y) $(pkg_m))

define PKG_PATH
  $(filter %/$(1), $(pkg_y) $(pkg_m))
endef

ifeq ($(Q),)
  define PKG_EXEC
	$(1) || exit $$?
  endef
else
  define PKG_EXEC
	$(1) > /dev/null 2>&1 || exit $$?
  endef
endif

define PKG_MAKE
  if [ -d $(call PKG_PATH,$(2)) ]; then \
	echo $(1) $(2); \
	$(call PKG_EXEC,$(MAKE) -C $(call PKG_PATH,$(2)) -I$(DIR_ROOT)/config -f $(FORMOSA_MK) $(3)); \
  fi;
endef

#
# add directory dependencies here
#
.PHONY: $(notdir $(pkg_y) $(pkg_m))

$(sort $(notdir $(pkg_y) $(pkg_m))):
	$(Q)$(call PKG_MAKE,"BUILD",$@)

romfs:
	$(Q)$(foreach i,$(sort $(pkg_y)),$(call PKG_MAKE,"ROMFS",$(notdir $i),romfs))
	$(Q)$(foreach i,$(sort $(pkg_m)),$(call PKG_MAKE,"EXTFS",$(notdir $i),romfs ROMFSINST=$(EXTFSINST)))

clean:
	$(Q)$(foreach i,$(sort $(pkg_y) $(pkg_m)),$(call PKG_MAKE,"CLEAN",$(notdir $i),clean))

%_tmpfs:
	$(Q)$(call PKG_MAKE,"TMPFS",$(@:_tmpfs=),tmpfs)

%_midfs:
	$(Q)$(call PKG_MAKE,"MIDFS",$(@:_midfs=),midfs)

%_romfs:
	$(Q)$(call PKG_MAKE,"ROMFS",$(@:_romfs=),romfs)

%_clean:
	$(Q)$(call PKG_MAKE,"CLEAN",$(@:_clean=),clean)

# ===========================================================================
# Kernel configuration targets
# These targets are used from top-level makefile

.PHONY: oldconfig menuconfig config silentoldconfig defconfig

Kconfig := Kconfig

menuconfig: $(DIR_ROOT)/config/$(VAR_HOST)/mconf
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/mconf $(Kconfig)

config: $(DIR_ROOT)/config/$(VAR_HOST)/mconf
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/mconf $(Kconfig)

defconfig: $(DIR_ROOT)/config/$(VAR_HOST)/conf
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/conf --alldefconfig $(Kconfig)

oldconfig: $(DIR_ROOT)/config/$(VAR_HOST)/conf
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/conf --silentoldconfig $(Kconfig)

silentoldconfig: $(DIR_ROOT)/config/$(VAR_HOST)/conf
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/conf --silentoldconfig $(Kconfig)
