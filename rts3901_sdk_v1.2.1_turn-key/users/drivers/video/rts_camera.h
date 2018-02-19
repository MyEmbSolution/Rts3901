/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTS_CAMERA_H
#define _RTS_CAMERA_H

#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "linux/rts_camera.h"

enum rtscam_size_type {
	RTSCAM_SIZE_DISCRETE = 0,
	RTSCAM_SIZE_STEPWISE = 1,
	RTSCAM_SIZE_CONTINUOUS = 2,
};

struct rtscam_frame_size {
	__u32 width;
	__u32 height;
};

struct rtscam_frame_frmival {
	struct v4l2_fract frmival;
	struct rtscam_frame_frmival *next;
};

struct rtscam_video_frmival {
	__u8 initialized;
	__u8 frmival_type;
	union {
		struct {
			struct rtscam_frame_frmival *frmivals;
		} discrete;
		struct {
			struct v4l2_fract max;
			struct v4l2_fract min;
			struct v4l2_fract step;
		} stepwise;
	};
};

struct rtscam_video_frame {
	struct rtscam_frame_size size;

	struct rtscam_video_frmival frmival;
	struct rtscam_video_frame *next;
};

struct rtscam_video_format {
	__u8 type;
	__u8 index;
	__u8 colorspace;
	__u8 field;
	const char name[32];
	__u32 fourcc;
	__u8 bpp;
	bool is_yuv;
	__u8 rts_code;

	__u8 initialized;

	__u8 frame_type;
	union {
		struct {
			struct rtscam_video_frame *frames;
		} discrete;
		struct {
			struct rtscam_frame_size max;
			struct rtscam_frame_size min;
			struct rtscam_frame_size step;
			struct rtscam_video_frmival frmival;
		} stepwise;
	};
	struct rtscam_video_format *next;
};

struct rtscam_video_format_xlate {
	__u8 type;
	__u8 index;
	__u8 colorspace;
	__u8 field;
	const char name[32];
	__u32 fourcc;
	__u8 bpp;
	bool is_yuv;
	__u8 rts_code;
};

struct rtscam_video_buffer {
	struct vb2_buffer		vb;
	struct list_head		list;
	__u32				pts;
};

struct rtscam_video_stream {
	struct rtscam_video_device *icd;

	__u8 streamid;	/*initialized by user*/

	struct video_device *vdev;
	struct vb2_queue vb2_vidp;
	struct v4l2_prio_state prio;		/* V4L2 priority state */
	atomic_t active;

	spinlock_t	lock;
	struct mutex stream_lock;		/* V4L2 priority state */
	struct mutex queue_lock;		/* V4L2 priority state */
	struct list_head capture;

	struct rtscam_video_format *user_formats;	/*initialized by user*/

	__u8 rts_code;
	__u32 user_format;
	__u32 user_width;
	__u32 user_height;
	__u32 user_numerator;
	__u32 user_denominator;

	__u32 bytesperline;
	__u32 sizeimage;

	unsigned long sequence;
	unsigned long overflow;

	atomic_t use_count;

	struct file *memory_owner;
};

struct rtscam_video_ctrl;

struct rtscam_video_ops {
	struct module *owner;

	int (*start_clock)(struct rtscam_video_device *icd);
	int (*stop_clock)(struct rtscam_video_device *icd);

	int (*get_ctrl)(struct rtscam_video_device *icd,
			struct rtscam_video_ctrl **ctrl, int index);
	int (*put_ctrl)(struct rtscam_video_device *icd,
			struct rtscam_video_ctrl *ctrl);
	int (*query_ctrl)(struct rtscam_video_device *icd, __u8 unit,
			__u8 query, __u8 selector, __u8 length, __u8 *data);

	int (*init_capture_buffers)(struct rtscam_video_stream *stream);
	int (*submit_buffer)(struct rtscam_video_stream *stream,
			struct rtscam_video_buffer *buffer);
	int (*s_stream)(struct rtscam_video_stream *stream, int enable);

	int (*exec_command)(struct rtscam_video_stream *stream,
			struct rtscam_vcmd *pcmd);
};

struct rtscam_video_device {
	struct v4l2_device v4l2_dev;
	struct device *dev;		/*initialized by user*/

	struct vb2_alloc_ctx    *alloc_ctx;	/*initialized by user*/
	const struct vb2_mem_ops *mem_ops;	/*initialized by user*/
	unsigned streamnum;		/*initialized by user*/
	struct rtscam_video_stream *streams;	/*initialized by user*/
	struct rtscam_video_ops *ops;	/*initialized by user*/
	const char *drv_name;	/*initialized by user*/
	const char *dev_name;	/*initialized by user*/
	int video_nr;		/*initialized by user*/
	u32 type;		/*initialized by user*/

	struct mutex dev_lock;

	struct mutex ctrl_lock;
	struct list_head controls;

	int streaming_count;

	atomic_t use_count;

	unsigned long flags;
	void *priv;	/*initialized by user*/
	int initialized;	/*initialized by user*/
};

static inline struct rtscam_video_device *to_rtscam_video_device(
		const struct device *dev)
{
	struct v4l2_device *v4l2_dev = dev_get_drvdata(dev);
	return container_of(v4l2_dev, struct rtscam_video_device, v4l2_dev);
}

static inline struct rtscam_video_buffer *to_rtscam_vbuf(struct vb2_buffer *vb)
{
	return container_of(vb, struct rtscam_video_buffer, vb);
}

enum rtscam_state {
	RTSCAM_STATE_PASSIVE		= 0,
	RTSCAM_STATE_ACTIVE		= 1
};

struct rtscam_fh {
	struct v4l2_fh vfh;
	enum rtscam_state state;
	struct rtscam_video_stream *stream;
};

/* Ctrl flags */
#define RTS_CTRL_FLAG_SET_CUR			(1 << 0)
#define RTS_CTRL_FLAG_GET_CUR			(1 << 1)
#define RTS_CTRL_FLAG_GET_MIN			(1 << 2)
#define RTS_CTRL_FLAG_GET_MAX			(1 << 3)
#define RTS_CTRL_FLAG_GET_RES			(1 << 4)
#define RTS_CTRL_FLAG_GET_LEN			(1 << 5)
#define RTS_CTRL_FLAG_GET_INFO			(1 << 6)
#define RTS_CTRL_FLAG_GET_DEF			(1 << 7)
/* Control should be saved at suspend and restored at resume. */
#define RTS_CTRL_FLAG_RESTORE			(1 << 8)
#define RTS_CTRL_FLAG_AUTO_UPDATE		(1 << 9)

#define RTS_CTRL_FLAG_GET_RANGE		\
	(RTS_CTRL_FLAG_GET_CUR | RTS_CTRL_FLAG_GET_MIN | \
	 RTS_CTRL_FLAG_GET_MAX | RTS_CTRL_FLAG_GET_RES | \
	 RTS_CTRL_FLAG_GET_DEF)

#define RTS_CTRL_DATA_TYPE_RAW			0
#define RTS_CTRL_DATA_TYPE_SIGNED		1
#define RTS_CTRL_DATA_TYPE_UNSIGNED		2
#define RTS_CTRL_DATA_TYPE_BOOLEAN		3
#define RTS_CTRL_DATA_TYPE_ENUM			4
#define RTS_CTRL_DATA_TYPE_BITMASK		5

#define RTS_CTRL_DATA_CURRENT			0
#define RTS_CTRL_DATA_BACKUP			1
#define RTS_CTRL_DATA_MIN			2
#define RTS_CTRL_DATA_MAX			3
#define RTS_CTRL_DATA_RES			4
#define RTS_CTRL_DATA_DEF			5
#define RTS_CTRL_DATA_LAST			6

#define RTS_CTRL_SET_CUR			0x01
#define RTS_CTRL_GET_CUR			0x81
#define RTS_CTRL_GET_MIN			0x82
#define RTS_CTRL_GET_MAX			0x83
#define RTS_CTRL_GET_RES			0x84
#define RTS_CTRL_GET_LEN			0x85
#define RTS_CTRL_GET_INFO			0x86
#define RTS_CTRL_GET_DEF			0x87

struct rtscam_video_ctrl_info {
	struct list_head mappings;
	__u8 index;
	__u8 unit;
	__u8 selector;
	__u8 size;
	__u32 flags;
};

struct rtscam_video_ctrl_menu {
	__u32 value;
	__u8 name[32];
};

struct rtscam_video_ctrl_mapping {
	struct list_head list;

	__u32 id;
	__u8 name[32];
	__u8 unit;
	__u8 selector;

	__u8 size;
	__u8 offset;
	enum v4l2_ctrl_type v4l2_type;
	__u32 data_type;

	struct rtscam_video_ctrl_menu *menu_info;
	__u32 menu_count;

	__s32 (*get)(struct rtscam_video_ctrl_mapping *mapping, __u8 query,
			const u8 *data);
	void (*set)(struct rtscam_video_ctrl_mapping *mapping, __s32 value,
			__u8 *data);
};

struct rtscam_video_ctrl {
	struct list_head list;
	__u8 unit;
	struct rtscam_video_ctrl_info info;

	__u8 index;
	__u8 dirty:1,
	     loaded:1,
	     modified:1,
	     cached:1,
	     initialized:1;

	__u8 *ctrl_data;
};

struct rtscam_ge_file_operations {
	struct module *owner;
	ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
	unsigned int (*poll)(struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
	int (*open)(struct file *);
	int (*release)(struct file *);
	int (*mmap)(struct file *, struct vm_area_struct *);
	int (*fasync)(int fd, struct file *filp, int mode);
};

struct rtscam_ge_device {
	struct rtscam_ge_file_operations *fops;
	struct device dev;
	struct cdev cdev;

	struct device *parent;

	int minor;
	unsigned long flags;

	char name[32];
	int debug;

	void (*release)(struct rtscam_ge_device *rdev);

	struct fasync_struct *async_queue;

	struct mutex lock;
};

#define RTSCAM_FL_REGISTERED		(0)

#define to_rtscam_ge_device(cd) container_of(cd, struct rtscam_ge_device, dev)

#define RTS_TRACE_ERROR		(1 << 0)
#define RTS_TRACE_PROBE		(1 << 1)
#define RTS_TRACE_CTRL		(1 << 2)
#define RTS_TRACE_IOCTL		(1 << 3)
#define RTS_TRACE_BUF		(1 << 4)
#define RTS_TRACE_VIDEO		(1 << 5)
#define RTS_TRACE_STATUS	(1 << 6)
#define RTS_TRACE_DEBUG		(1 << 7)
#define RTS_TRACE_INFO		(1 << 8)
#define RTS_TRACE_NOTICE	(1 << 9)
#define RTS_TRACE_WARNING	(1 << 10)
#define RTS_TRACE_REG		(1 << 11)
#define RTS_TRACE_MEMINFO	(1 << 12)
#define RTS_TRACE_DEV		(1 << 13)
#define RTS_TRACE_COMMAND	(1 << 14)
#define RTS_TRACE_POWER		(1 << 15)
#define RTS_TRACE_PTS		(1 << 16)

#define RTS_TRACE_DEFAULT	(RTS_TRACE_ERROR | RTS_TRACE_INFO)

extern unsigned int rtscam_debug;

#define rtsprintk(flag, arg...) \
	do {\
		if (rtscam_debug & (flag)) {\
			if ((flag) & RTS_TRACE_ERROR) \
				printk(KERN_ERR "rtscam:"arg);\
			else \
				printk(KERN_INFO "rtscam:"arg);\
		}\
	} while(0)

int rtscam_ctrl_add_info(struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_info *info);
void rtscam_ctrl_clr_info(struct rtscam_video_ctrl *ctrl);
int rtscam_ctrl_add_mapping(struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_mapping *mapping);
void rtscam_ctrl_clr_mapping(struct rtscam_video_ctrl *ctrl);

int rtscam_video_register_device(struct rtscam_video_device *icd);
int rtscam_video_unregister_device(struct rtscam_video_device *icd);

struct rtscam_ge_device *rtscam_ge_device_alloc(void);
void rtscam_ge_device_release(struct rtscam_ge_device *rdev);
struct rtscam_ge_device *rtscam_devdata(struct file *filp);

static inline void *rtscam_ge_get_drvdata(struct rtscam_ge_device *rdev)
{
	return dev_get_drvdata(&rdev->dev);
}

static inline void rtscam_ge_set_drvdata(struct rtscam_ge_device *rdev, void *data)
{
	dev_set_drvdata(&rdev->dev, data);
}

int rtscam_ge_register_device(struct rtscam_ge_device *rdev);
void rtscam_ge_unregister_device(struct rtscam_ge_device *rdev);

void rtscam_ge_kill_fasync(struct rtscam_ge_device *rdev, int sig, int band);

/**/
int rtscam_register_format(struct rtscam_video_stream *stream,
		struct rtscam_video_format_xlate *pfmt);
int rtscam_register_frame_discrete(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size);
int rtscam_register_frame_stepwise(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *max,
		struct rtscam_frame_size *min, struct rtscam_frame_size *step);
int rtscam_register_frmival_discrete(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size,
		struct v4l2_fract *ival);
int rtscam_register_frmival_stepwise(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size,
		struct v4l2_fract *max, struct v4l2_fract *min,
		struct v4l2_fract *step);
int rtscam_clr_format(struct rtscam_video_stream *stream);

int rtscam_get_phy_addr(struct vb2_buffer *vb, dma_addr_t *phy_addr);

int rts_test_bit(const __u8 *data, int bit);
void rts_clear_bit(__u8 *data, int bit);

int rtscam_submit_buffer(struct rtscam_video_stream *stream,
		struct rtscam_video_buffer *buf);
void rtscam_buffer_done(struct rtscam_video_stream *stream,
		struct rtscam_video_buffer *rbuf, unsigned long bytesused);

long rtscam_video_do_ctrl_ioctl(struct rtscam_video_device *icd,
		unsigned int cmd, void *arg);

typedef long (*rtscam_kioctl)(struct file *file, unsigned int cmd, void *arg);

long rtscam_usercopy(struct file *filp, unsigned int cmd, unsigned long arg,
		rtscam_kioctl func);

#define RTSCAM_L_BYTE(a)	((a) & 0xff)
#define RTSCAM_H_BYTE(a)	(((a) >> 8) & 0xff)
#define RTSCAM_L_WORD(a)	((a) & 0xffff)
#define RTSCAM_H_WORD(a)	(((a) >> 16) & 0xffff)

#endif

