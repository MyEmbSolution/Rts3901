#
# Realtek Semiconductor Corp.
#
# Tony Nie (tony_nie@realsil.com.cn)
# Mar. 19, 2015
#

.PHONY: all tmpfs

all: tmpfs

package_y =

package_$(CONFIG_PACKAGE_3rdparty_libaacenc) += libaacenc
package_$(CONFIG_PACKAGE_3rdparty_tutk) += tutk
package_$(CONFIG_PACKAGE_3rdparty_librtsaec) += librtsaec
package_$(CONFIG_PACKAGE_3rdparty_librtsmp3) += librtsmp3

romfs:
	for i in $(package_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done

tmpfs:
	for i in $(package_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i || exit $$? ; \
	done

