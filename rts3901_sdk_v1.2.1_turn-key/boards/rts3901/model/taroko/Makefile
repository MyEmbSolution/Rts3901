#
# Realtek Semiconductor Corp.
#
# Makefile
#	model Makefile for RLX Linux
#
# Viller Hsiao (villerhsiao@realtek.com)
# Aug. 26, 2013
# Jun-Ru Chang (jrjang@realtek.com)
# Oct. 21, 2013
#

.PHONY: romfs image

LINUX_VERSION := `cat $(DIR_ROOT)/.config | grep "CONFIG_LINUX_.*=y" | \
		  awk -F_ '{ print $$3; }' | awk -F= '{ print $$1; }'`

ENTRY_POINT_ADDR := `$(CROSS_COMPILE)readelf -h $(DIR_IMAGE)/vmlinux.elf | \
		     grep "Entry point address" | awk -Fx '{ print $$2; }'`

ENTRY_POINT_ADDR_Z := `$(CROSS_COMPILE)readelf -h $(DIR_IMAGE)/vmlinuz.elf | \
                     grep "Entry point address" | awk -Fx '{ print $$2; }'`

romfs:

image:
	$(Q)[ -d $(DIR_IMAGE) ] || mkdir -p $(DIR_IMAGE)
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(DIR_IMAGE)/vmlinux.elf $(DIR_IMAGE)/vmlinux.bin
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/mkimage \
		-n linux_$(LINUX_VERSION) \
		-A mips \
		-O linux \
		-T kernel \
		-C none \
		-a 0x80010000 \
		-e 0x$(ENTRY_POINT_ADDR) \
		-d $(DIR_IMAGE)/vmlinux.bin $(DIR_IMAGE)/vmlinux.img
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(DIR_IMAGE)/vmlinuz.elf $(DIR_IMAGE)/vmlinuz.bin
	$(Q)$(DIR_ROOT)/config/$(VAR_HOST)/mkimage \
                -n linux_$(LINUX_VERSION) \
                -A mips \
                -O linux \
                -T kernel \
                -C none \
                -a 0x$(ENTRY_POINT_ADDR_Z) \
                -e 0x$(ENTRY_POINT_ADDR_Z) \
                -d $(DIR_IMAGE)/vmlinuz.bin $(DIR_IMAGE)/vmlinuz.img
	$(Q)$(CROSS_COMPILE)objdump -D $(DIR_IMAGE)/vmlinux.elf > $(DIR_IMAGE)/vmlinux.txt
	$(Q)$(CROSS_COMPILE)objdump -D $(DIR_IMAGE)/vmlinuz.elf > $(DIR_IMAGE)/vmlinuz.txt
