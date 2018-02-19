BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S91aplayer /etc/rcS.d

clean:
