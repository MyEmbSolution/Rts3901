# Realtek Semiconductor Corp.
#
# bsp/Makefile
#     bsp Makefile
#
# Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
#

#
# Sheipa
#
CPPFLAGS_vmlinux.lds := $(KBUILD_CFLAGS)

core-y   += arch/rlx/bsp/
cflags-y += -I$(srctree)/arch/rlx/bsp/
load-y   += 0x80010000

extra-y  += vmlinux.lds
clean-files := *.o

obj-y := prom.o setup.o irq.o timer.o serial.o force-reset.o xb2.o
obj-$(CONFIG_PCI) += pci.o pci-fixup.o pci-ops.o
obj-$(CONFIG_HW_HAS_USB) += usb.o
obj-$(CONFIG_SMP) += smp.o
obj-$(CONFIG_8139GB) += 8139gb.o
obj-$(CONFIG_SPI) += spi_dev.o
obj-$(CONFIG_I2C_DESIGNWARE_PLATFORM) += i2c.o
obj-$(CONFIG_HAVE_CLK) += clock.o
obj-$(CONFIG_RTS_DMAC) += dmac.o
obj-$(CONFIG_SENSORS_RTS_SARADC) += saradc.o
obj-y += sd.o
obj-y += audio.o
obj-$(CONFIG_PINCTRL_RTS) += pinctrl.o
obj-$(CONFIG_LEDS_GPIO) += led.o
obj-$(CONFIG_RTL8168G) += rtl8168g.o
obj-$(CONFIG_DWC_OTG) += dwc_otg.o
obj-$(CONFIG_USB_RTS_PHY) += usb_phy.o
obj-y += isp.o
obj-$(CONFIG_RTS_EFUSE) += efuse.o
obj-$(CONFIG_RTSX_WATCHDOG) += watchdog.o
obj-$(CONFIG_PWM_RTSX) += pwm.o
obj-$(CONFIG_CRYPTO_DEV_RLX) += crypto.o
obj-$(CONFIG_MFD_RTP) += pmu.o
obj-$(CONFIG_RTC_DRV_RTS) += rtc.o
obj-$(CONFIG_RTS_THERMAL) += thermal.o
obj-$(CONFIG_SRAM) += ocsram.o
obj-$(CONFIG_RTS_CPUFREQ) += cpu_dvfs.o
