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
	$(TMPFSINST) librtsmp3.a /lib/librtsmp3.a
	$(TMPFSINST) librtsmp3.so /lib/librtsmp3.so

midfs:
	$(MIDFSINST) include /include
	$(MIDFSINST) librtsmp3.a /lib/librtsmp3.a
	$(MIDFSINST) librtsmp3.so /lib/librtsmp3.so

romfs:
	$(ROMFSINST) librtsmp3.so /lib/librtsmp3.so

clean:
