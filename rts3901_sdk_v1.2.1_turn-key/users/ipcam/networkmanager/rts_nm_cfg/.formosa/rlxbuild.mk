BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/nm_cfg /bin

clean:
