BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:

romfs:
	$(ROMFSINST) $(BUILD)/cgi-bin /usr/www/cgi-bin
	$(ROMFSINST) $(BUILD)/html /usr/www
	$(ROMFSINST) $(BUILD)/js /usr/www/js
	$(ROMFSINST) $(BUILD)/css /usr/www/css
	$(ROMFSINST) $(BUILD)/img /usr/www/img
	$(ROMFSINST) $(BUILD)/plugin /usr/www/plugin
	$(ROMFSINST) $(BUILD)/conf /etc/conf
	$(ROMFSINST) -s /media/sdcard /usr/www

clean:

