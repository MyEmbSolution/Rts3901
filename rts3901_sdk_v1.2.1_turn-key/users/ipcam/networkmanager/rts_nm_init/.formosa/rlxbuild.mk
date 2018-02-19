BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/nm_init /bin
	$(ROMFSINST) $(BUILD)/etc/conf/network.json /etc/conf
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S50nm_init /etc/rcS.d

clean:
