#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Aug. 21, 2014
#

BUILD = $(shell pwd)/.formosa/build
SOURCE_DIR = $(shell pwd)

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

.PHONY: all alsalib

all: alsalib tmpfs

alsalib:
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -f Makefile ] || \
	  ../../configure \
		--prefix=$(BUILD) \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--with-versioned=no \
		--with-configdir=/usr/share/alsa \
		--disable-python \
		--enable-static=no \
		--enable-shared=yes; \
	$(MAKE); \

tmpfs:
	mkdir -p $(DIR_TMPFS)/include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/asoundlib.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/asoundef.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/version.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/global.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/input.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/output.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/error.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/conf.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/pcm.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/rawmidi.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/timer.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/hwdep.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/control.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/mixer.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/seq_event.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/seq.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/seqmid.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/seq_midi_event.h /include/alsa
	$(TMPFSINST) $(SOURCE_DIR)/include/use-case.h /include/alsa
	$(TMPFSINST) $(BUILD)/src/.libs /lib

romfs:
	for x in $(shell cd $(BUILD)/src/.libs; ls *.so*); do \
		if [ -L $(BUILD)/lib/$$x ]; then \
			dest=`readlink $(BUILD)/src/.libs/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/src/.libs/$$x /lib; \
		fi; \
	done
	mkdir -p $(DIR_ROMFS)/usr/share/alsa
	mkdir -p $(DIR_ROMFS)/usr/share/alsa/cards
	mkdir -p $(DIR_ROMFS)/usr/share/alsa/pcm
	$(ROMFSINST) $(BUILD)/../../src/conf/alsa.conf /usr/share/alsa
	$(ROMFSINST) $(BUILD)/../../src/conf/cards/aliases.conf /usr/share/alsa/cards/aliases.conf
	$(ROMFSINST) $(BUILD)/../../src/conf/pcm/default.conf /usr/share/alsa/pcm/default.conf
	$(ROMFSINST) $(BUILD)/../../src/conf/pcm/dmix.conf /usr/share/alsa/pcm/dmix.conf
	$(ROMFSINST) $(BUILD)/../../src/conf/pcm/dsnoop.conf /usr/share/alsa/pcm/dsnoop.conf

clean:
	rm -rf $(BUILD)
