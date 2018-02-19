#
# Realtek Semiconductor Corp.
#
# Wind Han (wind_han@realsil.com.cn)
# Feb. 26, 2016
#

.PHONY: all

all: tmpfs midfs

tmpfs:
	$(TMPFSINST) include /include
	$(TMPFSINST) librtsaec.a /lib/librtsaec.a
	$(TMPFSINST) librtsaec.so /lib/librtsaec.so

midfs:
	$(MIDFSINST) include /include
	$(MIDFSINST) librtsaec.a /lib/librtsaec.a
	$(MIDFSINST) librtsaec.so /lib/librtsaec.so

romfs:
	$(ROMFSINST) librtsaec.so /lib/librtsaec.so

clean:
