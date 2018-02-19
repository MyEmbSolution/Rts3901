BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/autoipd /bin
	$(ROMFSINST) $(BUILD)/autoipd.action /var/conf

clean:
