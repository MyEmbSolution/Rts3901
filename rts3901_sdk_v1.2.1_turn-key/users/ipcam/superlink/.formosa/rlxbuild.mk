BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/super_link /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S92super_link /etc/rcS.d

clean:
