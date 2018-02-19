BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/*.so /lib
	$(ROMFSINST) $(BUILD)/mDNSResponderPosix /bin
	$(ROMFSINST) $(BUILD)/S95mDNSResponder /etc/rcS.d

clean:
