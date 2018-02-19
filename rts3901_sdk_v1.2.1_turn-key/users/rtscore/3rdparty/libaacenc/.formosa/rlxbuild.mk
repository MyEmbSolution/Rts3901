#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Mar. 18, 2015
#

.PHONY: all

all: tmpfs midfs

tmpfs:
	$(TMPFSINST) include /include
	$(TMPFSINST) libaacenc.so /lib/libaacenc.so

midfs:
	$(MIDFSINST) include /include
	$(MIDFSINST) libaacenc.so /lib/libaacenc.so

romfs:
	$(ROMFSINST) libaacenc.so /lib/libaacenc.so

clean:
