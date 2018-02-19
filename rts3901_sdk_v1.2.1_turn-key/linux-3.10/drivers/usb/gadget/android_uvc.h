#ifndef _ANDROID_UVC_H
#define _ANDROID_UVC_H

int rtsx_uvc_function_bind(struct usb_configuration *);
int rtsx_uvc_function_init(void);
void rtsx_uvc_function_cleanup(void);
void rtsx_uvc_function_enable(void);

#endif
