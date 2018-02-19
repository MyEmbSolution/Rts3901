BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin
	$(ROMFSINST) $(BUILD)/lib/libexplugin.so /usr/lib/doorkeeper/libexplugin.so
	$(ROMFSINST) $(BUILD)/lib/libdoorkeepermsg.so /lib/libdoorkeepermsg.so
	$(ROMFSINST) $(BUILD)/etc/conf/alarm.json /etc/conf
	$(ROMFSINST) $(BUILD)/etc/conf/ftp.json /etc/conf
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S94doorkeeper /etc/rcS.d
clean:
