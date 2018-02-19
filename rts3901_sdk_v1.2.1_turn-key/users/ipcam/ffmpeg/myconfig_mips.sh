#!/bin/sh


./configure \
	--enable-cross-compile \
	--cc=rsdk-linux-gcc \
	--ar=rsdk-linux-ar \
	--nm=rsdk-linux-nm \
	--strip=rsdk-linux-strip \
	--extra-cflags=-I/media/code/ipcam/install_alsa_lib/include \
	--extra-ldflags=-L/media/code/ipcam/install_alsa_lib/lib \
	--target-os=linux \
	--arch=mips \
	--disable-mips32r2 \
	--disable-mipsdspr1 \
	--disable-mipsdspr2 \
	--disable-mipsfpu \
	--prefix=/media/code/ipcam/install \
	--disable-shared \
	--enable-static \
	--enable-small \
	--disable-everything \
	--disable-debug \
	--disable-doc	\
	--disable-muxers \
	--disable-filters \
	--disable-devices \
	--disable-encoders \
	--disable-hwaccels \
	--enable-indev=v4l2 \
	--enable-indev=alsa \
	--enable-muxer=mjpeg \
	--enable-muxer=mp4 \
	--enable-muxer=rtp \
	--enable-muxer=ffm \
	--enable-muxer=shm \
	--enable-muxer=wav \
	--enable-muxer=aplayer \
	--enable-demuxer=ffm \
	--enable-demuxer=shm \
	--enable-demuxer=aplayer \
	--enable-protocols \
	--enable-decoder=h264 \
	--enable-decoder=adpcm_g726 \
	--enable-decoder=adpcm_g722 \
	--enable-decoder=aac \
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
	--enable-encoder=aac \
	--enable-parser=h264 \
	--enable-decoder=rawvideo \
	--enable-bsf=aac_adtstoasc \
	--enable-version3 \

#--disable-programs \
#--enable-parsers \
#--enable-demuxer=rtp \
#--enable-muxer=rawvideo \
#--enable-muxer=h264 \

#--disable-lsp \
#--disable-lzo \
#--disable-mdct \
#--disable-rdft \
#--disable-fft \
#--disable-dct \
#--disable-dwt \




#--enable-protocol=rtp \
#--enable-protocol=http \
#--enable-protocol=file \

