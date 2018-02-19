-include $(DIR_TMPFS)/appconfig.in
BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin/init_camera /bin
	$(ROMFSINST) $(BUILD)/bin/sync_osd_time /bin
	$(ROMFSINST) $(BUILD)/etc/conf /etc/conf
	$(ROMFSINST) $(BUILD)/etc/rcS.d /etc/rcS.d
	$(ROMFSINST) $(BUILD)/etc/double_biggest.bin /etc
	$(ROMFSINST) $(BUILD)/etc/double_big.bin /etc
	$(ROMFSINST) $(BUILD)/etc/double_med.bin /etc
	$(ROMFSINST) $(BUILD)/etc/double_small.bin /etc
	$(ROMFSINST) $(BUILD)/etc/single_biggest.bin /etc
	$(ROMFSINST) $(BUILD)/etc/single_big.bin /etc
	$(ROMFSINST) $(BUILD)/etc/single_med.bin /etc
	$(ROMFSINST) $(BUILD)/etc/single_small.bin /etc
	$(ROMFSINST) $(BUILD)/etc/chi.bin /etc
	$(ROMFSINST) $(BUILD)/etc/eng.bin /etc
clean:

