#
# Realtek Semiconductor Corp.
#
# Tony Nie (tony_nie@realsil.com.cn)
# Jul. 23, 2014
#

BUILD := $(shell pwd)/.formosa/build
FFMPEG = ffmpeg_target

.PHONY: all $(FFMPEG)

all: $(FFMPEG) tmpfs

$(FFMPEG):
	mkdir -p $(BUILD)
	cd $(BUILD); \
	[ -e config.h ] || \
	  ../../configure \
		--prefix=$(BUILD) \
		--enable-cross-compile \
		--cc=$(CC) \
		--ar=$(AR) \
		--nm=$(NM) \
		--strip=$(STRIP) \
		--target-os=linux \
		--arch=mips \
		--extra-cflags=-I$(DIR_TMPFS)/include \
		--extra-ldflags=-L$(DIR_TMPFS)/lib \
		--disable-iconv \
		--disable-mips32r2 \
		--disable-mipsdspr1 \
		--disable-mipsdspr2 \
		--disable-mipsfpu \
		--enable-shared \
		--enable-static \
		--enable-small \
		--disable-everything \
		--disable-programs \
		--disable-debug \
		--disable-doc	\
		--disable-muxers \
		--disable-filters \
		--disable-devices \
		--disable-encoders \
		--disable-hwaccels \
		--enable-indev=v4l2 \
		--enable-indev=rtstream_video \
		--enable-indev=rtstream_audio \
		--enable-outdev=rtstream_audio \
		--enable-indev=alsa \
		--enable-outdev=alsa \
		--enable-muxer=mjpeg \
		--enable-muxer=mp4 \
		--enable-muxer=rtp \
		--enable-muxer=ffm \
		--enable-demuxer=ffm \
		--enable-muxer=shm \
		--enable-demuxer=shm \
		--enable-muxer=amix \
		--enable-demuxer=amix \
		--enable-demuxer=pcm_s16le \
		--enable-muxer=wav \
		--enable-demuxer=wav \
		--enable-protocols \
		--enable-decoder=h264 \
		--enable-decoder=mjpeg \
		--enable-decoder=adpcm_g726 \
		--enable-decoder=adpcm_g722 \
		--enable-encoder=adpcm_g726 \
		--enable-encoder=adpcm_g722 \
		--enable-encoder=pcm_mulaw \
		--enable-encoder=pcm_alaw \
		--enable-decoder=pcm_mulaw \
		--enable-decoder=pcm_alaw \
		--enable-encoder=pcm_s32le \
		--enable-encoder=pcm_s32be \
		--enable-encoder=pcm_s24le \
		--enable-encoder=pcm_s24be \
		--enable-encoder=pcm_s16le \
		--enable-encoder=pcm_s16be \
		--enable-encoder=pcm_s8 \
		--enable-encoder=pcm_u32le \
		--enable-encoder=pcm_u32be \
		--enable-encoder=pcm_u24le \
		--enable-encoder=pcm_u24be \
		--enable-encoder=pcm_u16le \
		--enable-encoder=pcm_u16be \
		--enable-encoder=pcm_u8 \
		--enable-decoder=pcm_s32le \
		--enable-decoder=pcm_s32be \
		--enable-decoder=pcm_s24le \
		--enable-decoder=pcm_s24be \
		--enable-decoder=pcm_s16le \
		--enable-decoder=pcm_s16be \
		--enable-decoder=pcm_s8 \
		--enable-decoder=pcm_u32le \
		--enable-decoder=pcm_u32be \
		--enable-decoder=pcm_u24le \
		--enable-decoder=pcm_u24be \
		--enable-decoder=pcm_u16le \
		--enable-decoder=pcm_u16be \
		--enable-decoder=pcm_u8 \
		--enable-parser=h264 \
		--enable-decoder=rawvideo \
		--enable-bsf=aac_adtstoasc \
		--enable-libh1encoder \
		--enable-encoder=rsht \
		--enable-librtsjpeg \
		--enable-encoder=rtsjpg \
		--enable-libaacenc \
		--enable-encoder=rtsaac \
		--enable-encoder=libopencore_amrnb \
		--enable-libopencore-amrnb \
		--enable-decoder=librtsmp3 \
		--enable-librtsmp3 \
		--enable-version3; \
		$(MAKE); \
		$(MAKE) install;

#--enable-indev=alsa \
#--enable-encoder=aac \
#--enable-decoder=aac \

tmpfs:
	$(TMPFSINST) $(BUILD)/include /include
	$(TMPFSINST) $(BUILD)/lib /lib
romfs:
	for x in $(shell cd $(BUILD)/lib; ls *.so*); do \
		if [ -L $(BUILD)/lib/$$x ]; then \
			dest=`readlink $(BUILD)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(BUILD)/lib/$$x /lib; \
		fi; \
	done

clean:
	rm -rf $(BUILD)
