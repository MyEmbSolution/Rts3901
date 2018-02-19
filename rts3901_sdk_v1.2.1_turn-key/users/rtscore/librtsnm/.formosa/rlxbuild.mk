BUILD = $(shell pwd)/.formosa/build

.PHONY: all

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib/librtsnm.so /lib

romfs:
	$(ROMFSINST) $(BUILD)/include /include
	$(ROMFSINST) $(BUILD)/lib/librtsnm.so /lib

clean:

