#
# Makefile for DWC_otg Highspeed USB controller driver
#

ifeq ($(CONFIG_DWC_OTG_DEBUG),y)
EXTRA_CFLAGS   += -DDEBUG
endif

ifeq ($(CONFIG_DWC_OTG_DEBUG_VERBOSE),y)
EXTRA_CFLAGS   += -DVERBOSE
endif

# Use one of the following flags to compile the software in host-only or
# device-only mode.
ifeq ($(CONFIG_DWC_OTG_HOST_ONLY),y)
EXTRA_CFLAGS   += -DDWC_HOST_ONLY
EXTRA_CFLAGS   += -DDWC_EN_ISOC
endif

ifeq ($(CONFIG_DWC_OTG_DEVICE_ONLY),y)
EXTRA_CFLAGS   += -DDWC_DEVICE_ONLY
endif


# how to do with DWC_UTE_CFI DWC_UTE_PER_IO DWC_EN_ISOC CONFIG_USB_DWC_OTG_LPM

# Use the BUS_INTERFACE variable to compile the software for either 
# PCI(PCI_INTERFACE) or LM(LM_INTERFACE) bus.
#ifeq ($(BUS_INTERFACE),)
#	BUS_INTERFACE = -DPCI_INTERFACE
#	BUS_INTERFACE = -DLM_INTERFACE
#endif

EXTRA_CFLAGS	+= -Dlinux -DDWC_HS_ELECT_TST
#EXTRA_CFLAGS	+= -DDWC_EN_ISOC
EXTRA_CFLAGS   	+= -I$(PWD)/dwc_common_port
#EXTRA_CFLAGS   += -I$(PORTLIB)
EXTRA_CFLAGS   	+= -DDWC_LINUX
EXTRA_CFLAGS   	+= $(CFI)
#EXTRA_CFLAGS	+= $(BUS_INTERFACE)
#EXTRA_CFLAGS	+= -DDWC_DEV_SRPCAP


obj-$(CONFIG_DWC_OTG)	:= dwc_otg.o
obj-$(CONFIG_DWC_OTG)	+= dwc_common_port/

dwc_otg-objs	:= dwc_otg_driver.o dwc_otg_attr.o
dwc_otg-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o
dwc_otg-objs	+= dwc_otg_pcd_linux.o dwc_otg_pcd.o dwc_otg_pcd_intr.o
dwc_otg-objs	+= dwc_otg_hcd.o dwc_otg_hcd_linux.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o dwc_otg_hcd_ddma.o
dwc_otg-objs	+= dwc_otg_adp.o
ifneq ($(CFI),)
dwc_otg-objs	+= dwc_otg_cfi.o
endif
