BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/lib /lib
	$(TMPFSINST) $(BUILD)/include /include

romfs:
	$(ROMFSINST) $(BUILD)/lib/libpemsg.so /lib
	$(ROMFSINST) $(BUILD)/bin/peacock /bin
	$(ROMFSINST) $(BUILD)/bin/snapshot_widget /bin
	$(ROMFSINST) $(BUILD)/bin/del_sdcard_files /bin
	$(ROMFSINST) $(BUILD)/bin/rtspd /bin
	$(ROMFSINST) $(BUILD)/bin/record_widget /bin
	$(ROMFSINST) $(BUILD)/bin/start_peacock.sh /bin
	$(ROMFSINST) $(BUILD)/bin/alsad /bin
	$(ROMFSINST) $(BUILD)/bin/lark /bin

	$(ROMFSINST) $(BUILD)/etc/rcS.d/S90peacock /etc/rcS.d
	$(ROMFSINST) $(BUILD)/etc/conf/peacock.json /etc/conf


clean:

