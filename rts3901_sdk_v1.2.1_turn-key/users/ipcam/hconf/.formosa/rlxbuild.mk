BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/bin/hconf /bin
	$(ROMFSINST) $(BUILD)/etc/hconf.ini /etc

clean:
