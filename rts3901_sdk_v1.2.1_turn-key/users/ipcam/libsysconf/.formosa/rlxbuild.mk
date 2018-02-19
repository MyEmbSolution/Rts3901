BUILD = $(shell pwd)/.formosa/build
SHARED_LIB_SYSCONF = libsysconf.so
STATIC_LIB_SYSCONF = libsysconf.a

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/lib/$(SHARED_LIB_SYSCONF) /lib
	$(TMPFSINST) $(BUILD)/lib/$(STATIC_LIB_SYSCONF) /lib
	$(TMPFSINST) $(BUILD)/include/sysconf.h /include

romfs:
	$(ROMFSINST) $(BUILD)/lib/$(SHARED_LIB_SYSCONF) /lib
	$(ROMFSINST) $(BUILD)/bin/entropy /bin/
	$(ROMFSINST) $(BUILD)/bin/check_cfg /bin/


clean:
