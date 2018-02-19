DIR_SOURCE := $(shell pwd)/mDNSPosix
BUILD := $(shell pwd)/.formosa/build
OBJECTS := $(shell pwd)/.formosa/objects

.PHONY: all

all:
	mkdir -p $(BUILD); \
	cd $(DIR_SOURCE); \
	cp ./S95mDNSResponder $(BUILD); \
	$(MAKE) os=linux; \

romfs:
	$(ROMFSINST) $(BUILD)/mDNSResponderPosix /bin/
	$(ROMFSINST) $(BUILD)/*.so /lib/
	$(ROMFSINST) $(BUILD)/S95mDNSResponder /etc/rcS.d

clean:
	-rm -rf $(BUILD) $(OBJECTS)

