#!/bin/sh

./configure \
	--enable-version3 \
	--extra-cflags="-O0 -g" \
	--extra-libs=-lc \
	--target-os=linux \
	--prefix=/media/code/ipcam/install_x86 \
	--enable-debug \
	--enable-static \
	--disable-shared \
	--disable-yasm \
	--disable-optimizations \
	--disable-mmx \
	--disable-stripping \
	--enable-gpl \
	--disable-ffserver \
	--disable-ffmpeg \
	--disable-ffplay \
	--disable-ffprobe \
	--disable-encoder=rsht \
	--enable-libx264 \
	--enable-decoder=h264 \
	--enable-libopencore-amrwb \
	--enable-libopencore-amrnb \


#--disable-static \
#--enable-shared \
#--extra-ldflags=-static \
#--disable-shared \
#--enable-static \

#--disable-everything \
#--enable-indev=v4l2 \
#--enable-indev=alsa \
#--enable-muxer=h264 \
#--enable-muxer=mp4 \
#--enable-muxer=rtp \
#--enable-muxer=rtsp \
#--enable-protocol=rtp \
#--enable-protocol=http \
#--enable-protocol=file \
#--enable-parser=h264 \
#--enable-encoder=mpeg4 \
#--enable-encoder=aac \




#--enable-encoder=libx264 \
#--disable-mipsfpu
#--disable-decoders \
#--disable-encoders \
#--disable-muxers \
#--disable-demuxers \
#--disable-parsers \
#--disable-protocols \
#--disable-filters \
#--disable-outdevs \
#--disable-indevs \
#--disable-bsfs \


