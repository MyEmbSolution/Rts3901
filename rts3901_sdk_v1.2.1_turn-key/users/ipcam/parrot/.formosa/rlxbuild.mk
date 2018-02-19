BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/parrot /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S91parrot /etc/rcS.d


clean:

