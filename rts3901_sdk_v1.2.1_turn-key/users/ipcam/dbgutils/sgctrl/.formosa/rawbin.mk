BUILD = $(shell pwd)/.formosa/build

all:

romfs:
	$(ROMFSINST) $(BUILD)/bin /bin

clean:

