#ifndef _UVC_H264_H_
#define _UVC_H264_H_

int32_t uvc_h264enc_close();
int32_t uvc_h264enc_frame(uint8_t *fyuv, uint8_t **f264buf, uint32_t *f264len);
int32_t uvc_h264enc_init(int32_t width, int32_t height, int32_t fps);
uint32_t getoutbufsize();
uint32_t getoutbufstart();

#endif
