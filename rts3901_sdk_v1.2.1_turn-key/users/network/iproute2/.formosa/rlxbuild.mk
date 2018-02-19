#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Sep. 15, 2011
#

all:
	$(MAKE)

romfs:
	$(ROMFSINST) build/iproute2 /bin/iproute2

clean:
	$(MAKE) clean
