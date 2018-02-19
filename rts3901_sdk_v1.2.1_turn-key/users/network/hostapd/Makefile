#
#realtek Semiconductor Corp.
#
# hurray_niu (hurray_niu@realsil.com.cn)
# Aug. 21, 2014
#

hostapd-y :=
hostapd-$(CONFIG_PACKAGE_hostapd) += hostapd
supplicant-y :=
supplicant-$(CONFIG_PACKAGE_supplicant) += wpa_supplicant

all:
	for i in $(hostapd-y); \
                do $(MAKE) -C $$i; \
        done
	for i in $(supplicant-y); \
                do $(MAKE) -C $$i; \
        done

romfs:
	@for i in $(hostapd-y); \
                do $(MAKE) -C $$i romfs; \
        done
	@for i in $(supplicant-y); \
                do $(MAKE) -C $$i romfs; \
        done

clean:
	@for i in $(hostapd-y); \
                do $(MAKE) -C $$i clean; \
        done
	@for i in $(supplicant-y); \
                do $(MAKE) -C $$i clean; \
        done

