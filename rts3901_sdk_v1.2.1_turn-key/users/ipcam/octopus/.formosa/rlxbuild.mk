BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include/octopus
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/lib/liboctopus.so /lib
	$(ROMFSINST) $(BUILD)/bin/octopus /bin

clean:

