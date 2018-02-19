BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S94uvcd /etc/rcS.d

clean:
