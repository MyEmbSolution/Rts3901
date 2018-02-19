BUILD = $(shell pwd)/.formosa/build

all: tmpfs

tmpfs:

romfs:
	mkdir -p $(DIR_ROMFS)/$(ONVIF_CGI_DST)
	$(ROMFSINST) $(ONVIF_SERVICE_SO) /lib/$(ONVIF_SERVICE_SO)
	$(ROMFSINST) device_discovery /bin/device_discovery
	$(ROMFSINST) onvif_events_submgr /bin/onvif_events_submgr
	$(ROMFSINST) device_service   $(ONVIF_CGI_DST)/device_service
	$(ROMFSINST) S96onvif	/etc/rcS.d

clean:
	-rm -f *.o  device_discovery device_service onvif_evnets_submgr $(ONVIF_SERVICE_SO)

