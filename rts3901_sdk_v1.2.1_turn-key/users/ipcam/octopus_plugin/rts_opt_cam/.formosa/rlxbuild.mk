BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/lib/librtscam.so /lib
	$(TMPFSINST) $(BUILD)/lib/libopt_cam.so /lib/opt_cam.so
	$(TMPFSINST) $(BUILD)/include/rts_cam.h /include/octopus

romfs:
	$(ROMFSINST) $(BUILD)/lib/librtscam.so /lib
	$(ROMFSINST) $(BUILD)/lib/libopt_cam.so /usr/lib/octopus/opt_cam.so
	$(ROMFSINST) $(BUILD)/etc/rcS.d /etc/rcS.d

clean:
