BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S99isptuning /etc/rcS.d

clean:

