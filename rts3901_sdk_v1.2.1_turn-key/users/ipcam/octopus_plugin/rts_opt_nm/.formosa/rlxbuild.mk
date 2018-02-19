BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/include /include
	$(ROMFSINST) $(BUILD)/lib/librtsnm.so /lib
	$(ROMFSINST) $(BUILD)/lib/libopt_nm.so /usr/lib/octopus/opt_nm.so
	$(ROMFSINST) $(BUILD)/etc/rcS.d /etc/rcS.d

clean:
