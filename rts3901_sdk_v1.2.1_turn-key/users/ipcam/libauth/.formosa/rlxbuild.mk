BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:
	$(TMPFSINST) $(BUILD)/lib/libauth.so /lib/
	$(TMPFSINST) $(BUILD)/lib/libauth.a /lib/
	$(TMPFSINST) $(BUILD)/include /include/auth/

romfs:
	mkdir -p $(DIR_ROMFS)/etc/conf
	mkdir -p $(DIR_ROMFS)/etc/rcS.d
	$(ROMFSINST) $(BUILD)/lib/libauth.so /lib
	$(ROMFSINST) $(BUILD)/bin/autotest /bin
	$(ROMFSINST) $(BUILD)/etc/conf/pub_key /etc/conf
	$(ROMFSINST) $(BUILD)/etc/conf/private_key /etc/conf
	$(ROMFSINST) $(BUILD)/etc/conf/user_info.dat /etc/conf
	$(ROMFSINST) $(BUILD)/etc/rcS.d/S40rtsauth /etc/rcS.d

clean:
