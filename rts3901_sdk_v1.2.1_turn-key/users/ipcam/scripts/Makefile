mdev-y :=
mdev-$(CONFIG_PACKAGE_scripts_automount) += automount.sh
network-y :=
network-$(CONFIG_PACKAGE_scripts_udhcpc) += default.script

.PHONY: romfs

all:
	echo "Empty target"

romfs:
	mkdir -p $(DIR_ROMFS)/lib/mdev
	mkdir -p $(DIR_ROMFS)/usr/share/udhcpc
	for i in $(mdev-y); \
		do $(ROMFSINST) $$i /lib/mdev/$$i; \
	done
	for i in $(network-y); \
		do $(ROMFSINST) $$i /usr/share/udhcpc/$$i; \
	done

clean:
	echo "cleanup scripts"
