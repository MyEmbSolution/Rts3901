BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/cam_finder /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S94camfinder /etc/rcS.d

clean:
