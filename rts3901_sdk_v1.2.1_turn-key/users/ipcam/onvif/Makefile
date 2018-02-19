#
# Realtek Semiconductor Corp.
#
# yafei meng (yafei_meng@realsil.com.cn)
# Aug. 14, 2014
#

ONVIF_CGI_DST		= /usr/www/onvif
ONVIF_DEVICE_DISCOVERY	= device_discovery
ONVIF_DEVICE_SERVICE	= device_service
ONVIF_SERVICE_SO	= librsOnvifService.so
ONVIF_EVENT_SUBMGR	= onvif_events_submgr

SOAP_CFLAGS = -DWITH_NOIDREF
#CFLAGS = -g -Wall -DDEBUG
CFLAGS += -I$(DIR_TMPFS)/include -I.
CFLAGS += -fpic

DEP_LIBS += -lm -lrt -lpthread -ljson-c -loctopus -lrtsuvc -lrtsnm
#DEP_LIBS += -lm -lrt -lpthread -ljson-c -loctopus -lrtsuvc -lrtsnm -lrtsio
#DEP_LIBS += -lm -lrt -lpthread -ljson-c -loctopus -lrtscam -lrtsnm -lrtsio
LDLIBS := -L$(DIR_TMPFS)/lib $(LDLIBS) $(DEP_LIBS)


OnvifServiceObjs	= soapC.o stdsoap2.o soapServer.o soapClient.o rsOnvifConfig.o duration.o wsaapi.o threads.o wsddapi.o rsOnvifCommonFunc.o rsOnvifMsg.o rsOnvifDevCtrl.o rsOnvifRWJsonConfig.o rsOnvifSystemCtrl.o rsOnvifCmdDevmgmt.o rsOnvifCmdEvent.o rsOnvifCmdMedia.o rsOnvifCmdImaging.o rsOnvifCmdOthers.o rsOnvifWsddEvent.o rsOnvifEventMgrLib.o
DeviceDiscoveryObj	= rsOnvifDeviceDiscovery.o
DeviceServiceObj	= rsOnvifDeviceService.o
EventSubmgrObj		= rsOnvifEventManager.o
OpensslObj		= rsOnvifOpenssl.o
OpensslSrc		= rsOnvifOpenssl.c


all:$(ONVIF_DEVICE_DISCOVERY) $(ONVIF_DEVICE_SERVICE) $(ONVIF_EVENT_SUBMGR) tmpfs

$(ONVIF_DEVICE_DISCOVERY):$(ONVIF_SERVICE_SO) $(DeviceDiscoveryObj)
	$(CC) $(SOAP_CFLAGS) -o $@ $^ -L. -lrsOnvifService $(LDLIBS)

$(ONVIF_DEVICE_SERVICE):$(ONVIF_SERVICE_SO) $(DeviceServiceObj)
	$(CC) $(SOAP_CFLAGS) -o $@ $^ -L. -lrsOnvifService $(LDLIBS)

$(ONVIF_EVENT_SUBMGR):$(ONVIF_SERVICE_SO) $(EventSubmgrObj)
	$(CC) $(SOAP_CFLAGS) -o $@ $^ -L. -lrsOnvifService $(LDLIBS)

$(ONVIF_SERVICE_SO):$(OpensslObj) $(OnvifServiceObjs)
	$(CC) -shared $(CFLAGS) $(SOAP_CFLAGS) -o $@ $^ $(LDLIBS)

$(OpensslObj):$(OpensslSrc)
	$(CC) -c -std=c99 $(CFLAGS) -o $@ $^

tmpfs:
	echo "do nothing"

romfs:
	mkdir -p $(DIR_ROMFS)/$(ONVIF_CGI_DST)
	$(ROMFSINST) $(ONVIF_SERVICE_SO) /lib/$(ONVIF_SERVICE_SO)
	$(ROMFSINST) device_discovery /bin/device_discovery
	$(ROMFSINST) onvif_events_submgr /bin/onvif_events_submgr
	$(ROMFSINST) device_service   $(ONVIF_CGI_DST)/device_service

.PHONY:clean
clean:
	-rm -f *.o  device_discovery device_service onvif_evnets_submgr $(ONVIF_SERVICE_SO)

