BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/nm_wps /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S51nm_wps /etc/rcS.d

clean:
