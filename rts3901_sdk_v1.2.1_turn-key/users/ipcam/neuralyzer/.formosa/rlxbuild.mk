BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/bin/neuralyzer /bin
	$(ROMFSINST) $(BUILD)/bin/neucleanup /bin
	$(ROMFSINST) $(BUILD)/lib/libneuralyzer.so /lib

clean:
