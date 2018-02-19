#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Mar. 18, 2015
#

include common.mk

.PHONY: all

TUTK_LIB_PATH = $(shell pwd)/lib

all: tmpfs midfs

tmpfs:
	$(TMPFSINST) include/AVAPIs.h /include
	$(TMPFSINST) include/AVFRAMEINFO.h /include
	$(TMPFSINST) include/AVIOCTRLDEFs.h /include
	$(TMPFSINST) include/IOTCAPIs.h /include
	$(TMPFSINST) include/P2PTunnelAPIs.h /include
	$(TMPFSINST) include/RDTAPIs.h /include
	$(TMPFSINST) $(TUTK_LIB_PATH) /lib

midfs:
	$(MIDFSINST) include /include
	$(call INST_SO, $(TUTK_LIB_PATH), /lib, $(MIDFSINST))

romfs:
	$(call INST_SO, $(TUTK_LIB_PATH), /lib, $(ROMFSINST))

clean:
