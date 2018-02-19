/*
#define _UVC_1V5_
#define DEBUG
#define LOAD_FROM_IMAGE_FILE
#define DUMP_H264_FILE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <semaphore.h>
#include <poll.h>
#include <rtsisp.h>
#include "h264enc.h"
#include "testuvc.h"

#define TRANSFER_SIZE (32 * 1024)

void video_thread();
void probe_commit();
void videocontrolreq();
void errno_exit(const int8_t *s);
void start_stuff();
void send_epc_respond();


uint8_t fpsarray[] = {
	120, 60, 30, 25, 24, 23, 20, 15, 12, 11, 10, 9, 8, 7, 5, 3
};

struct ctl_t ctl_exposuretimeabsolut;
struct ctl_t ctl_exposuretimeauto;
struct ctl_t ctl_lowlightcomp;
struct ctl_t ctl_brightness;
struct ctl_t ctl_contrast;
struct ctl_t ctl_hue;
struct ctl_t ctl_saturation;
struct ctl_t ctl_sharpness;
struct ctl_t ctl_gamma;
struct ctl_t ctl_whitebalancetemp;
struct ctl_t ctl_backlightcomp;
struct ctl_t ctl_gain;
struct ctl_t ctl_pwrlinefreq;
struct ctl_t ctl_whitebalancetempauto;
struct ctl_t ctl_pantilt;
struct ctl_t ctl_zoom;
struct ctl_t ctl_roll;
struct ctl_t ctl_trapeziumcorrection;
#ifdef _AF_ENABLE_
struct ctlitemu16_t focusabsolutitem;
struct ctlitemu8_t focusautoitem;
struct ctl_t ctl_focusabsolut;
struct ctl_t ctl_focusauto;
#endif

struct ctlitems8_t tcorrectionitem;
struct ctlitemu32_t exposuretimeabsolutitem;
struct ctlitemu8_t exposuretimeautoitem;
struct ctlitemu8_t lowlightcompitem;
struct ctlitems16_t brightnessitem;
struct ctlitemu16_t contrastitem;
struct ctlitems16_t hueitem;
struct ctlitemu16_t saturationitem;
struct ctlitemu16_t gammaitem;
struct ctlitemu16_t whitebalancetempitem;
struct ctlitemu16_t backlightcompitem;
struct ctlitemu16_t gainitem;
struct ctlitemu8_t pwrlinefreqitem;
struct ctlitemu8_t whitebalancetempautoitem;
struct ctlitems16_t panitem;
struct ctlitems16_t tiltitem;
struct ctlitemu16_t zoomitem;
struct ctlitems16_t rollitem;
struct ctlitemu16_t sharpnessitem;

uint16_t g_wextuispctl;
uint16_t g_wcamtrmctlsel;
uint16_t g_wprocunitctlsel;
uint8_t g_bycamtrmctlsel_ext;

struct v4l2_event event_now;
struct v4l2_event event_setup;
struct v4l2_event event_data;

struct uvc_request_data respond;
struct uvc_request_data respond2;

static int32_t usbfd = -1;
static int32_t capturefd_m = -1;
static int32_t capturefd_yuv420 = -1;
static int32_t capturefd = -1;
static int32_t android_fd = -1;

#ifdef LOAD_FROM_IMAGE_FILE
struct buffer *usbbuffers;
#endif
struct buffer *capturebuffers;

static int32_t global_stop;
static int8_t usb_dev[25];
static int8_t capture_dev_mjpeg[25];
static int8_t capture_dev_yuv420[25];
static int8_t capture_dev[25];

static int32_t g_width;
static int32_t g_height;
static int32_t g_fps;
static int32_t g_type;
struct vsprobcommstruct g_vsprobe;
struct vsprobcommstruct g_vscommit;

int32_t ep0outdataindex;

int32_t ep0indataindex;
uint8_t ep0indatabuffer[2048];

int32_t epcindataindex;
uint8_t epcindatabuffer[16];

uint8_t g_brequesttype;
uint8_t g_brequest;
uint16_t g_wvalue;
uint16_t g_windex;
uint16_t g_wlength;
uint8_t g_wvalue_h;
uint8_t g_wvalue_l;
uint8_t g_windex_h;
uint8_t g_windex_l;
uint8_t g_setup_phrase;

int32_t g_dwpanfalsevalue_forxpbug;
int32_t videostart;
pthread_t eventthid, videothid;

static uint8_t qualificationstring[8] = "REALSIL";
static uint8_t fwcompdatestring[9] = "20150130";
static uint16_t g_wI2CRegAddr;
static uint8_t l_byI2CSuperCmdFlag;
static uint8_t mainloop = 1;

uint16_t g_wdatainoutctlsize = MAX_RWFLASH_SIZE;

uint8_t g_byhwid1;
uint8_t g_byhwid2;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct format_setting commitfs;
uint8_t g_byvclasterror;

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)

#define container_of(p, t, n) (t *)(p)

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

LIST_HEAD(capturelist);
LIST_HEAD(usblist);

struct frame_list {
	struct list_head list;
	struct v4l2_buffer v4l2buf;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = 0;
	entry->prev = 0;
}

static inline int32_t list_empty(const struct list_head *head)
{
	return head->next == head;
}

void errno_exit(const int8_t *s)
{
	DPRINT("%s error %d, %s\n", s, errno, strerror(errno));
	exit(-1);
}

int32_t xioctl(int32_t fh, int32_t request, void *arg)
{
	int32_t r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

uint8_t ep0_data_out()
{
	return event_data.u.data[ep0outdataindex++];
}

void ep0_data_in(uint8_t byte)
{
	ep0indatabuffer[ep0indataindex++] = byte;
}

void assign_byte(uint8_t *addr, int32_t i, uint8_t value)
{
	addr[i] = value;
}

void epc_data_in(uint8_t byte)
{
	epcindatabuffer[epcindataindex++] = byte;
}

int32_t rts_read_xmem(int32_t dfd, uint16_t addr, uint8_t len, uint8_t *data)
{
	int fd = rts_isp_ctrl_open();
	struct rts_isp_mem_rw mem_rw;
	int ret;

	if (fd < 0)
		return fd;

	mem_rw.addr = addr;
	mem_rw.length = len;
	mem_rw.pdata = data;

	ret = rts_isp_read_xmem(fd, &mem_rw);

	rts_isp_ctrl_close(fd);

	return ret;
}

int32_t rts_write_xmem(int32_t dfd, uint16_t addr, uint8_t len, uint8_t *data)
{
	int fd = rts_isp_ctrl_open();
	struct rts_isp_mem_rw mem_rw;
	int ret;

	if (fd < 0)
		return fd;

	mem_rw.addr = addr;
	mem_rw.length = len;
	mem_rw.pdata = data;

	ret = rts_isp_write_xmem(fd, &mem_rw);

	rts_isp_ctrl_close(fd);

	return ret;
}

static __query_ctrl_info(struct v4l2_queryctrl *queryctrl)
{
	DPRINT("Ctrl id:0x%08x\n" \
	       "type:0x%08x\n" \
	       "name:%s\n	min:%d\n" \
	       "max:%d\n	step:%d\n" \
	       "default:%d\n	flags:0x%08x\n",
	       queryctrl->id,
	       queryctrl->type,
	       (int8_t *) queryctrl->name,
	       queryctrl->minimum,
	       queryctrl->maximum,
	       queryctrl->step, queryctrl->default_value, queryctrl->flags);
}

int32_t list_video_ctrls(int32_t fd)
{
	struct v4l2_queryctrl queryctrl;
	queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
	while (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
		__query_ctrl_info(&queryctrl);
		queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}
	return 0;
}

int32_t query_video_ctrl(int32_t fd, uint32_t id, struct v4l2_queryctrl *queryctrl)
{
	int32_t ret;

	queryctrl->id = id;
	ret = ioctl(fd, VIDIOC_QUERYCTRL, queryctrl);

	return ret;
}

int32_t check_video_ctrl(int32_t fd, int32_t id)
{
	struct v4l2_queryctrl queryctrl;
	queryctrl.id = id;
	return ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
}

int32_t get_video_ctrl(int32_t fd, uint32_t id)
{
	struct v4l2_control ctrl;
	int32_t ret;

	ret = check_video_ctrl(fd, id);
	if (ret) {
		DPRINT("ctrl (0x%x) is invalid, get failed\n", id);
		return ret;
	}
	ctrl.id = id;
	ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
	if (ret) {
		DPRINT("get ctrl (0x%x) failed : %d\n", id, ret);
		return ret;
	}
	DPRINT("Ctrl id = 0x%x, value = %d\n", id, ctrl.value);

	return	ctrl.value;

}

int32_t set_video_ctrl(int32_t fd, uint32_t id, int32_t value)
{
	struct v4l2_control ctrl;
	int32_t ret;

	ret = check_video_ctrl(fd, id);
	if (ret) {
		DPRINT("ctrl (0x%x) is invalid, set failed\n", id);
		return ret;
	}
	ctrl.id = id;
	ctrl.value = value;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret)
		DPRINT("set ctrl(id = 0x%x, value = %d) failed : %d\n",
		       id, value, ret);
	else
		DPRINT("set ctrl(id = 0x%x, value = %d) succeed\n", id, value);
	return ret;
}

void vcstatusintpacket_int0(uint8_t const originator,
			    uint8_t const cs,
			    uint8_t const attribute,
			    uint8_t const *const pvalue, uint8_t const size)
{
	epc_data_in(0x01);
	epc_data_in(originator);
	epc_data_in(0x00);
	epc_data_in(cs);
	epc_data_in(attribute);

	if (size == 1) {
		epc_data_in(pvalue[0]);
	} else if (size == 2) {
		epc_data_in(pvalue[0]);
		epc_data_in(pvalue[1]);

	} else {
		epc_data_in(pvalue[0]);
		epc_data_in(pvalue[1]);
		epc_data_in(pvalue[2]);
		epc_data_in(pvalue[3]);
	}
	send_epc_respond();
}

void stall_epctl(uint8_t dir)
{
	if (g_setup_phrase) {
		respond.dir = dir;
		respond.stall = 1;
		DPRINT("stall_epctl\n");
		if (-1 == xioctl(usbfd, UVCIOC_SEND_RESPONSE, &respond)) {
			switch (errno) {
			case EAGAIN:
				return;
			case EIO:
			default:
				errno_exit("stall_epctl");
			}
		}
	}

	return;
}

void vcstall_epctl(uint8_t err, uint8_t dir)
{
	g_byvclasterror = err;
	stall_epctl(dir);
}

void send_respond()
{

	memcpy(respond.data, ep0indatabuffer, ep0indataindex);
	respond.length = ep0indataindex;
	respond.dir = 0;
	respond.stall = 0;

	ep0indataindex = 0;

	DPRINT("send_respond\n");
	if (-1 == xioctl(usbfd, UVCIOC_SEND_RESPONSE, &respond)) {
		switch (errno) {
		case EAGAIN:
			return;
		case EIO:
		default:
			errno_exit("send_respond");
		}
	}

	return;
}

void send_epc_respond()
{

	memcpy(respond2.data, epcindatabuffer, epcindataindex);
	respond2.length = epcindataindex;
	epcindataindex = 0;

	DPRINT("send_epc_respond\n");
	if (-1 == xioctl(usbfd, UVCIOC_SEND_RESPONSE_INT, &respond2)) {
		switch (errno) {
		case EAGAIN:
			return;
		case EIO:
		default:
			errno_exit("send_epc_respond");
		}
	}

	return;
}

void send_out_respond()
{
	respond.length = ((g_wlength <= 60) ? g_wlength : 60);
	respond.dir = 1;
	respond.stall = 0;

	DPRINT("send out respond\n");
	if (-1 == xioctl(usbfd, UVCIOC_SEND_RESPONSE, &respond)) {
		switch (errno) {
		case EAGAIN:
			return;
		case EIO:
		default:
			errno_exit("send_respond");
		}
	}

	return;
}

uint32_t queryfps(uint8_t fmtidx, uint8_t frmidx)
{
	struct format_setting tmpfs;

	tmpfs.width = fmtidx;
	tmpfs.height = frmidx;
	if (xioctl(android_fd, ANDROIDUVC_IOC_QUERYFORMAT, &tmpfs) != 0)
		errno_exit("ANDROIDUVC_IOC_QUERYFORMAT");

	return tmpfs.fps;
}

/*
 * Return the bit position (0..63) of the most significant 1 bit in a word
 * Returns -1 if no 1 bit exists
 */
static inline unsigned long __fls(unsigned long word)
{
	int32_t num;

	num = 32 - 1;

	if (!(word & (~0ul << (32 - 16)))) {
		num -= 16;
		word <<= 16;
	}
	if (!(word & (~0ul << (32 - 8)))) {
		num -= 8;
		word <<= 8;
	}
	if (!(word & (~0ul << (32 - 4)))) {
		num -= 4;
		word <<= 4;
	}
	if (!(word & (~0ul << (32 - 2)))) {
		num -= 2;
		word <<= 2;
	}
	if (!(word & (~0ul << (32 - 1))))
		num -= 1;
	return num;
}

/*
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Returns 0..SZLONG-1
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline unsigned long __ffs(unsigned long word)
{
	return __fls(word & -word);
}

void videostreamprobepacket(struct vsprobcommstruct const *const pvsctl,
			    uint8_t const req, uint8_t const isnegotiating)
{
	uint8_t i, j = 0;
	uint8_t fmtidx, frmidx;
	uint32_t dwtmp, fps;

	ep0_data_in(((uint8_t *) (&pvsctl->bmhint))[0]);
	ep0_data_in(((uint8_t *) (&pvsctl->bmhint))[1]);
	ep0_data_in(pvsctl->bformatindex);
	ep0_data_in(pvsctl->bframeindex);

	fmtidx = pvsctl->bformatindex;
	frmidx = pvsctl->bframeindex;

	if (isnegotiating) {
		fps = queryfps(fmtidx, frmidx);
		DPRINT("VS COMMIT fps:%d\n", fps);

		switch (req) {
		case GET_MIN:
#ifdef	LOAD_FROM_IMAGE_FILE
			dwtmp = 10000000 / g_fps;
#else
			dwtmp = 10000000 / fpsarray[__fls(fps)];
#endif
			break;
		case GET_MAX:
#ifdef	LOAD_FROM_IMAGE_FILE
			dwtmp = 10000000 / g_fps;
#else
			dwtmp = 10000000 / fpsarray[__ffs(fps)];

#endif
			break;
		case GET_DEF:
#ifdef	LOAD_FROM_IMAGE_FILE
			dwtmp = 10000000 / g_fps;
#else
			dwtmp = 10000000 / fpsarray[__ffs(fps)];

#endif
			break;
		case GET_CUR:
		default:
			dwtmp = pvsctl->dwframeinterval;
			break;
		}
	} else {
		dwtmp = pvsctl->dwframeinterval;
	}

	ep0_data_in(((uint8_t *) (&dwtmp))[0]);
	ep0_data_in(((uint8_t *) (&dwtmp))[1]);
	ep0_data_in(((uint8_t *) (&dwtmp))[2]);
	ep0_data_in(((uint8_t *) (&dwtmp))[3]);

	for (i = 0; i < 8; i++)
		ep0_data_in(0);

	ep0_data_in(32);
	ep0_data_in(0);
	ep0_data_in(((uint8_t *) (&pvsctl->dwmaxvideoframesize))[0]);
	ep0_data_in(((uint8_t *) (&pvsctl->dwmaxvideoframesize))[1]);
	ep0_data_in(((uint8_t *) (&pvsctl->dwmaxvideoframesize))[2]);
	ep0_data_in(((uint8_t *) (&pvsctl->dwmaxvideoframesize))[3]);
	ep0_data_in(((uint8_t *)
		     (&pvsctl->dwmaxpayloadtransfersize))[0]);
	ep0_data_in(((uint8_t *)
		     (&pvsctl->dwmaxpayloadtransfersize))[1]);
	ep0_data_in(((uint8_t *)
		     (&pvsctl->dwmaxpayloadtransfersize))[2]);
	ep0_data_in(((uint8_t *)
		     (&pvsctl->dwmaxpayloadtransfersize))[3]);

#ifdef _UVC_1V5_
	dwtmp = DEV_CLOCK_FRQ;

	ep0_data_in(((uint8_t *) (&dwtmp))[0]);
	ep0_data_in(((uint8_t *) (&dwtmp))[1]);
	ep0_data_in(((uint8_t *) (&dwtmp))[2]);
	ep0_data_in(((uint8_t *) (&dwtmp))[3]);

	ep0_data_in(7);
	ep0_data_in(0);
	ep0_data_in(0);
	ep0_data_in(0);
	ep0_data_in(0);
	ep0_data_in(8);
	ep0_data_in(0);
	ep0_data_in(0);
	ep0_data_in(0);
	ep0_data_in(0);

	for (i = 0; i < 8; i++)
		ep0_data_in(0);
#endif
	send_respond();

	return;
}

void videostreamreq(uint8_t const req, uint8_t const cs)
{

	static uint8_t bysisnegotiatefps;
	uint8_t i;
	uint8_t bmhint_l;
	uint8_t bmhint_h;
	uint8_t fmtidx;
	uint8_t frmidx;
	uint8_t fpsidx;
	uint8_t bytmp;
	uint32_t dwtmp;
	struct vsprobcommstruct vsprobecommittmp;
	uint8_t fpstmp;

	if (req == SET_CUR) {
		int32_t j;

		assign_byte((uint8_t *) &vsprobecommittmp.bmhint,
			    0, ep0_data_out());
		assign_byte((uint8_t *) &vsprobecommittmp.bmhint,
			    1, ep0_data_out());

		fmtidx = ep0_data_out();
		frmidx = ep0_data_out();

		vsprobecommittmp.bformatindex = fmtidx;
		vsprobecommittmp.bframeindex = frmidx;

		assign_byte((uint8_t *) &dwtmp, 0, ep0_data_out());
		assign_byte((uint8_t *) &dwtmp, 1, ep0_data_out());
		assign_byte((uint8_t *) &dwtmp, 2, ep0_data_out());
		assign_byte((uint8_t *) &dwtmp, 3, ep0_data_out());

		vsprobecommittmp.dwframeinterval = dwtmp;

		if (cs == VS_PROBE_CONTROL) {
			if (vsprobecommittmp.dwframeinterval == 0)
				bysisnegotiatefps = 1;
			else
				bysisnegotiatefps = 0;
		}

		struct format_setting tmpfs;

		tmpfs.width = fmtidx;
		tmpfs.height = frmidx;
		if (xioctl(android_fd, ANDROIDUVC_IOC_QUERYFORMAT, &tmpfs) != 0)
			errno_exit("ANDROIDUVC_IOC_QUERYFORMAT");

		vsprobecommittmp.dwmaxvideoframesize =
		    tmpfs.width * tmpfs.height * 2;
		vsprobecommittmp.dwmaxpayloadtransfersize = TRANSFER_SIZE;
		if (cs == VS_PROBE_CONTROL)
			memcpy(&g_vsprobe, &vsprobecommittmp,
			       sizeof(struct vsprobcommstruct));
		else {
			memcpy(&g_vscommit, &vsprobecommittmp,
			       sizeof(struct vsprobcommstruct));

			DPRINT("VS COMMIT\n");
#ifdef PREVIEW_PROBE_COMMIT
			get_ready_for_preview();
#endif

		}
	} else {
		DPRINT("GET_CUR\n");

		if (cs == VS_PROBE_CONTROL) {
			videostreamprobepacket(&g_vsprobe,
					       req, bysisnegotiatefps);
			bysisnegotiatefps = 0;
		} else {
			videostreamprobepacket(&g_vscommit, req, 0);
		}
	}
}

void videostreamifreq()
{
	switch (g_wvalue_h) {
	case VS_PROBE_CONTROL:{
			switch (g_brequest) {
			case GET_LEN:
#ifndef _UVC_1V5_
				ep0_data_in(CTL_SIZE_VS_PROBE_1V0);
#else
				ep0_data_in(CTL_SIZE_VS_PROBE_1V5);
#endif
				send_respond();
				break;
			case GET_INFO:
				ep0_data_in(CTL_INFO_VS_PROBE);
				send_respond();
				break;
			case GET_DEF:
			case GET_MAX:
			case GET_MIN:
			case GET_RES:
			case GET_CUR:
				videostreamreq(g_brequest, VS_PROBE_CONTROL);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				videostreamreq(g_brequest, VS_PROBE_CONTROL);
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
			break;
		}
	case VS_COMMIT_CONTROL:{
			switch (g_brequest) {
			case GET_LEN:
#ifndef _UVC_1V5_
				ep0_data_in(CTL_SIZE_VS_PROBE_1V0);
#else
				ep0_data_in(CTL_SIZE_VS_PROBE_1V5);
#endif
				send_respond();
				break;
			case GET_INFO:
				ep0_data_in(CTL_INFO_VS_COMMIT);
				send_respond();
				break;
			case GET_CUR:
				videostreamreq(g_brequest, VS_COMMIT_CONTROL);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}

				videostreamreq(g_brequest, VS_COMMIT_CONTROL);
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
			break;
		}
	case VS_STILL_PROBE_CONTROL:
	case VS_STILL_COMMIT_CONTROL:
	case VS_STILL_IMAGE_TRIGGER_CONTROL:
		vcstall_epctl(VC_ERR_INVDREQ, 0);
		return;
	case VS_STREAM_ERROR_CODE_CONTROL:{
			switch (g_brequest) {
			case GET_INFO:
				ep0_data_in(CTL_INFO_VS_ERROR_CODE);
				send_respond();
				break;
			case GET_CUR:
				ep0_data_in(VS_ERR_NOERROR);
				send_respond();
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
			break;
		}
	default:{
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			break;
		}
	}
	return;
}

void videostreamingreq()
{
	switch (g_windex_h) {
	case 0x00:
		videostreamifreq();
		break;
	default:{
			vcstall_epctl(VC_ERR_INVDUNIT, 0);
			break;
		}
	}
	return;

}

void videoclsreqproc()
{
	switch (g_brequesttype & 0x83) {
	case 0x81:
	case 0x01:{
			switch (g_windex_l) {
			case IF_IDX_VIDEOCONTROL:
				videocontrolreq();
				break;
			case IF_IDX_VIDEOSTREAMING:
				videostreamingreq();
				break;
			default:{
					stall_epctl(0);
					break;
				}
			}
			break;
		}
	default:{
			stall_epctl(0);
			break;
		}
	}
	return;
}

void initcustomizedvars(void)
{
	int ret;

#ifdef _AF_ENABLE_
	g_wcamtrmctlsel = CT_CTL_SEL_AUTO_EXP_MODE
	    | CT_CTL_SEL_AUTO_EXP_PRY
	    | CT_CTL_SEL_EXPOSURE_TIME_ABS
	    | CT_CTL_SEL_PANTILT_ABS
	    | CT_CTL_SEL_ZOOM_ABS | CT_CTL_SEL_ROLL_ABS | CT_CTL_SEL_FOCUS_ABS;
	g_bycamtrmctlsel_ext = CT_CTL_SEL_FOCUS_AUTO;
#else
	g_wcamtrmctlsel = CT_CTL_SEL_AUTO_EXP_MODE
	    | CT_CTL_SEL_AUTO_EXP_PRY
	    | CT_CTL_SEL_EXPOSURE_TIME_ABS
	    | CT_CTL_SEL_PANTILT_ABS
	    | CT_CTL_SEL_ZOOM_ABS | CT_CTL_SEL_ROLL_ABS;
	g_bycamtrmctlsel_ext = 0;
#endif

	g_wprocunitctlsel = PU_CTL_SEL_BRIGHTNESS
	    | PU_CTL_SEL_CONTRAST
	    | PU_CTL_SEL_HUE
	    | PU_CTL_SEL_SATURATION
	    | PU_CTL_SEL_SHARPNESS
	    | PU_CTL_SEL_GAMMA
	    | PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE
	    | PU_CTL_SEL_BACKLIGHT_COMPENSATION
	    | PU_CTL_SEL_POWER_LINE_FREQUENCY
	    | PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE_AUTO;


	struct v4l2_queryctrl queryctrl;

	query_video_ctrl(capturefd, V4L2_CID_BRIGHTNESS, &queryctrl);
	brightnessitem.max = queryctrl.maximum;
	brightnessitem.min = queryctrl.minimum;
	brightnessitem.res = queryctrl.step;
	brightnessitem.des = brightnessitem.def = queryctrl.default_value;
	brightnessitem.last = get_video_ctrl(capturefd, V4L2_CID_BRIGHTNESS);

	query_video_ctrl(capturefd, V4L2_CID_CONTRAST, &queryctrl);
	contrastitem.max = queryctrl.maximum;
	contrastitem.min = queryctrl.minimum;
	contrastitem.res = queryctrl.step;
	contrastitem.des = contrastitem.def = queryctrl.default_value;
	contrastitem.last = get_video_ctrl(capturefd, V4L2_CID_CONTRAST);

	query_video_ctrl(capturefd, V4L2_CID_HUE, &queryctrl);
	hueitem.max = queryctrl.maximum;
	hueitem.min = queryctrl.minimum;
	hueitem.res = queryctrl.step;
	hueitem.des = hueitem.def = queryctrl.default_value;
	hueitem.last = get_video_ctrl(capturefd, V4L2_CID_HUE);

	query_video_ctrl(capturefd, V4L2_CID_SATURATION, &queryctrl);
	saturationitem.max = queryctrl.maximum;
	saturationitem.min = queryctrl.minimum;
	saturationitem.res = queryctrl.step;
	saturationitem.des = saturationitem.def = queryctrl.default_value;
	saturationitem.last = get_video_ctrl(capturefd, V4L2_CID_SATURATION);


	query_video_ctrl(capturefd, V4L2_CID_GAMMA, &queryctrl);
	gammaitem.max = queryctrl.maximum;
	gammaitem.min = queryctrl.minimum;
	gammaitem.res = queryctrl.step;
	gammaitem.des = gammaitem.def = queryctrl.default_value;
	gammaitem.last = get_video_ctrl(capturefd, V4L2_CID_GAMMA);

	query_video_ctrl(capturefd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, &queryctrl);
	whitebalancetempitem.max = queryctrl.maximum;
	whitebalancetempitem.min = queryctrl.minimum;
	whitebalancetempitem.res = queryctrl.step;
	whitebalancetempitem.des = whitebalancetempitem.def = queryctrl.default_value;
	whitebalancetempitem.last = get_video_ctrl(capturefd, V4L2_CID_WHITE_BALANCE_TEMPERATURE);

	query_video_ctrl(capturefd, V4L2_CID_AUTO_WHITE_BALANCE, &queryctrl);
	whitebalancetempautoitem.max = queryctrl.maximum;
	whitebalancetempautoitem.min = queryctrl.minimum;
	whitebalancetempautoitem.res = queryctrl.step;
	whitebalancetempautoitem.des = whitebalancetempautoitem.def = queryctrl.default_value;
	whitebalancetempautoitem.last = get_video_ctrl(capturefd, V4L2_CID_AUTO_WHITE_BALANCE);

	query_video_ctrl(capturefd, V4L2_CID_BACKLIGHT_COMPENSATION, &queryctrl);
	backlightcompitem.max = queryctrl.maximum;
	backlightcompitem.min = queryctrl.minimum;
	backlightcompitem.res = queryctrl.step;
	backlightcompitem.des = backlightcompitem.def = queryctrl.default_value;
	backlightcompitem.last = get_video_ctrl(capturefd, V4L2_CID_BACKLIGHT_COMPENSATION);

	query_video_ctrl(capturefd, V4L2_CID_GAIN, &queryctrl);
	gainitem.max = queryctrl.maximum;
	gainitem.min = queryctrl.minimum;
	gainitem.res = queryctrl.step;
	gainitem.des = gainitem.def = queryctrl.default_value;
	gainitem.last = get_video_ctrl(capturefd, V4L2_CID_GAIN);

	query_video_ctrl(capturefd, V4L2_CID_POWER_LINE_FREQUENCY, &queryctrl);
	pwrlinefreqitem.max = queryctrl.maximum;
	pwrlinefreqitem.min = queryctrl.minimum;
	pwrlinefreqitem.res = queryctrl.step;
	pwrlinefreqitem.des = pwrlinefreqitem.def = queryctrl.default_value;
	pwrlinefreqitem.last = get_video_ctrl(capturefd, V4L2_CID_POWER_LINE_FREQUENCY);

	query_video_ctrl(capturefd, V4L2_CID_EXPOSURE_ABSOLUTE, &queryctrl);
	exposuretimeabsolutitem.max = queryctrl.maximum;
	exposuretimeabsolutitem.min = queryctrl.minimum;
	exposuretimeabsolutitem.res = queryctrl.step;
	exposuretimeabsolutitem.des = exposuretimeabsolutitem.def = queryctrl.default_value;
	exposuretimeabsolutitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO, &queryctrl);
	exposuretimeautoitem.max = queryctrl.maximum;
	exposuretimeautoitem.min = queryctrl.minimum;
	exposuretimeautoitem.res = queryctrl.step;
	exposuretimeautoitem.des = exposuretimeautoitem.def = queryctrl.default_value;
	exposuretimeautoitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO);

	query_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO_PRIORITY, &queryctrl);
	lowlightcompitem.max = queryctrl.maximum;
	lowlightcompitem.min = queryctrl.minimum;
	lowlightcompitem.res = queryctrl.step;
	lowlightcompitem.des = lowlightcompitem.def = queryctrl.default_value;
	lowlightcompitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO_PRIORITY);

	query_video_ctrl(capturefd, V4L2_CID_PAN_ABSOLUTE, &queryctrl);
	panitem.max = queryctrl.maximum;
	panitem.min = queryctrl.minimum;
	panitem.res = queryctrl.step;
	panitem.des = panitem.def = queryctrl.default_value;
	panitem.last = get_video_ctrl(capturefd, V4L2_CID_PAN_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_TILT_ABSOLUTE, &queryctrl);
	tiltitem.max = queryctrl.maximum;
	tiltitem.min = queryctrl.minimum;
	tiltitem.res = queryctrl.step;
	tiltitem.des = tiltitem.def = queryctrl.default_value;
	tiltitem.last = get_video_ctrl(capturefd, V4L2_CID_TILT_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_ZOOM_ABSOLUTE, &queryctrl);
	zoomitem.max = queryctrl.maximum;
	zoomitem.min = queryctrl.minimum;
	zoomitem.res = queryctrl.step;
	zoomitem.des = zoomitem.def = queryctrl.default_value;
	zoomitem.last = get_video_ctrl(capturefd, V4L2_CID_ZOOM_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_ROLL_ABSOLUTE, &queryctrl);
	rollitem.max = queryctrl.maximum;
	rollitem.min = queryctrl.minimum;
	rollitem.res = queryctrl.step;
	rollitem.des = rollitem.def = queryctrl.default_value;
	rollitem.last = get_video_ctrl(capturefd, V4L2_CID_ROLL_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_SHARPNESS, &queryctrl);
	sharpnessitem.max = queryctrl.maximum;
	sharpnessitem.min = queryctrl.minimum;
	sharpnessitem.res = queryctrl.step;
	sharpnessitem.des = rollitem.def = queryctrl.default_value;
	sharpnessitem.last = get_video_ctrl(capturefd, V4L2_CID_ROLL_ABSOLUTE);

	ctl_exposuretimeabsolut.info = CONTROL_INFO_SP_SET
	    | CONTROL_INFO_SP_GET
	    | CONTROL_INFO_DIS_BY_AUTO | CONTROL_INFO_SP_AUTOUPDATA;
	ctl_exposuretimeabsolut.changeflag = CTL_CHNGFLG_RDY;
	ctl_exposuretimeabsolut.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF;
	ctl_exposuretimeabsolut.len = CONTROL_LEN_4;
	ctl_exposuretimeabsolut.pattr =
	    (union ctlattr_t *)&exposuretimeabsolutitem;

	ctl_exposuretimeauto.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_exposuretimeauto.changeflag = CTL_CHNGFLG_RDY;
	ctl_exposuretimeauto.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_LEN | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF;
	ctl_exposuretimeauto.len = CONTROL_LEN_1;
	ctl_exposuretimeauto.pattr = (union ctlattr_t *)&exposuretimeautoitem;

	ctl_lowlightcomp.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_lowlightcomp.changeflag = CTL_CHNGFLG_RDY;
	ctl_lowlightcomp.opmask = CONTROL_OP_SET_CUR | CONTROL_OP_GET_LEN
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_RES
	    | CONTROL_OP_GET_DEF
	    | CONTROL_OP_GET_MIN | CONTROL_OP_GET_MAX | CONTROL_ATTR_UNSIGNED;
	ctl_lowlightcomp.len = CONTROL_LEN_1;
	ctl_lowlightcomp.pattr = (union ctlattr_t *)&lowlightcompitem;

	ctl_brightness.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_brightness.changeflag = CTL_CHNGFLG_RDY;
	ctl_brightness.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_SIGNED;
	ctl_brightness.len = CONTROL_LEN_2;
	ctl_brightness.pattr = (union ctlattr_t *)&brightnessitem;

	ctl_contrast.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_contrast.changeflag = CTL_CHNGFLG_RDY;
	ctl_contrast.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_contrast.len = CONTROL_LEN_2;
	ctl_contrast.pattr = (union ctlattr_t *)&contrastitem;

	ctl_hue.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_hue.changeflag = CTL_CHNGFLG_RDY;
	ctl_hue.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_SIGNED;
	ctl_hue.len = CONTROL_LEN_2;
	ctl_hue.pattr = (union ctlattr_t *)&hueitem;

	ctl_saturation.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_saturation.changeflag = CTL_CHNGFLG_RDY;
	ctl_saturation.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_saturation.len = CONTROL_LEN_2;
	ctl_saturation.pattr = (union ctlattr_t *)&saturationitem;

	ctl_sharpness.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_sharpness.changeflag = CTL_CHNGFLG_RDY;
	ctl_sharpness.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_sharpness.len = CONTROL_LEN_2;
	ctl_sharpness.pattr = (union ctlattr_t *)&sharpnessitem;

	ctl_gamma.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_gamma.changeflag = CTL_CHNGFLG_RDY;
	ctl_gamma.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_gamma.len = CONTROL_LEN_2;
	ctl_gamma.pattr = (union ctlattr_t *)&gammaitem;

	ctl_whitebalancetemp.info = CONTROL_INFO_SP_SET
	    | CONTROL_INFO_SP_GET
	    | CONTROL_INFO_DIS_BY_AUTO | CONTROL_INFO_SP_AUTOUPDATA;
	ctl_whitebalancetemp.changeflag = CTL_CHNGFLG_RDY;
	ctl_whitebalancetemp.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_LEN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_whitebalancetemp.len = CONTROL_LEN_2;
	ctl_whitebalancetemp.pattr = (union ctlattr_t *)&whitebalancetempitem;

	ctl_backlightcomp.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_backlightcomp.changeflag = CTL_CHNGFLG_RDY;
	ctl_backlightcomp.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_backlightcomp.len = CONTROL_LEN_2;
	ctl_backlightcomp.pattr = (union ctlattr_t *)&backlightcompitem;

	ctl_gain.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_gain.changeflag = CTL_CHNGFLG_RDY;
	ctl_gain.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_gain.len = CONTROL_LEN_2;
	ctl_gain.pattr = (union ctlattr_t *)&gainitem;

	ctl_pwrlinefreq.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_pwrlinefreq.changeflag = CTL_CHNGFLG_RDY;
	ctl_pwrlinefreq.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_RES
	    | CONTROL_OP_GET_DEF
	    | CONTROL_OP_GET_MIN | CONTROL_OP_GET_MAX | CONTROL_ATTR_UNSIGNED;
	ctl_pwrlinefreq.len = CONTROL_LEN_1;
	ctl_pwrlinefreq.pattr = (union ctlattr_t *)&pwrlinefreqitem;

	ctl_whitebalancetempauto.info = CONTROL_INFO_SP_SET
	    | CONTROL_INFO_SP_GET;
	ctl_whitebalancetempauto.changeflag = CTL_CHNGFLG_RDY;
	ctl_whitebalancetempauto.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_LEN | CONTROL_OP_GET_DEF | CONTROL_OP_GET_RES;
	ctl_whitebalancetempauto.len = CONTROL_LEN_1;
	ctl_whitebalancetempauto.pattr =
	    (union ctlattr_t *)&whitebalancetempautoitem;

	ctl_pantilt.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_pantilt.changeflag = CTL_CHNGFLG_RDY;
	ctl_pantilt.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_SIGNED;
	ctl_pantilt.len = CONTROL_LEN_8;
	ctl_pantilt.pattr = NULL;

	ctl_zoom.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_zoom.changeflag = CTL_CHNGFLG_RDY;
	ctl_zoom.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_UNSIGNED;
	ctl_zoom.len = CONTROL_LEN_2;
	ctl_zoom.pattr = (union ctlattr_t *)&zoomitem;

	ctl_roll.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_roll.changeflag = CTL_CHNGFLG_RDY;
	ctl_roll.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF | CONTROL_ATTR_SIGNED;
	ctl_roll.len = CONTROL_LEN_2;
	ctl_roll.pattr = (union ctlattr_t *)&rollitem;

	ctl_trapeziumcorrection.info = CONTROL_INFO_SP_SET
	    | CONTROL_INFO_SP_GET;
	ctl_trapeziumcorrection.changeflag = CTL_CHNGFLG_RDY;
	ctl_trapeziumcorrection.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_RES
	    | CONTROL_OP_GET_DEF | CONTROL_ATTR_SIGNED | CONTROL_OP_GET_LEN;
	ctl_trapeziumcorrection.len = CONTROL_LEN_1;
	ctl_trapeziumcorrection.pattr = (union ctlattr_t *)&tcorrectionitem;

	g_vsprobe.bmhint = 0;
	g_vsprobe.bformatindex = 1;
	g_vsprobe.bframeindex = 1;
#ifdef LOAD_FROM_IMAGE_FILE
	g_vsprobe.dwframeinterval = 10000000 / g_fps;
	g_vsprobe.dwmaxvideoframesize = g_width * g_height * 2;
#else
	g_vsprobe.dwframeinterval = 10000000 / 30;
	g_vsprobe.dwmaxvideoframesize = 640 * 480 * 2;
#endif

#ifdef _AF_ENABLE_
	query_video_ctrl(capturefd, V4L2_CID_FOCUS_ABSOLUTE, &queryctrl);
	focusabsolutitem.max = queryctrl.maximum;
	focusabsolutitem.min = queryctrl.minimum;
	focusabsolutitem.res = queryctrl.step;
	focusabsolutitem.des = focusabsolutitem.def = queryctrl.default_value;
	focusabsolutitem.last = get_video_ctrl(capturefd, V4L2_CID_FOCUS_ABSOLUTE);

	query_video_ctrl(capturefd, V4L2_CID_FOCUS_AUTO, &queryctrl);
	focusautoitem.max = queryctrl.maximum;
	focusautoitem.min = queryctrl.minimum;
	focusautoitem.res = queryctrl.step;
	focusautoitem.des = focusautoitem.def = queryctrl.default_value;
	focusautoitem.last = get_video_ctrl(capturefd, V4L2_CID_FOCUS_AUTO);

	ctl_focusabsolut.info = CONTROL_INFO_SP_SET
	    | CONTROL_INFO_SP_GET
	    | CONTROL_INFO_DIS_BY_AUTO | CONTROL_INFO_SP_AUTOUPDATA;
	ctl_focusabsolut.changeflag = CTL_CHNGFLG_RDY;
	ctl_focusabsolut.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_MIN | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF;
	ctl_focusabsolut.len = CONTROL_LEN_2;
	ctl_focusabsolut.pattr = (union ctlattr_t *)&focusabsolutitem;
	ctl_focusauto.info = CONTROL_INFO_SP_SET | CONTROL_INFO_SP_GET;
	ctl_focusauto.changeflag = CTL_CHNGFLG_RDY;
	ctl_focusauto.opmask = CONTROL_OP_SET_CUR
	    | CONTROL_OP_GET_CUR
	    | CONTROL_OP_GET_MIN
	    | CONTROL_OP_GET_MAX
	    | CONTROL_OP_GET_LEN | CONTROL_OP_GET_RES | CONTROL_OP_GET_DEF;
	ctl_focusauto.len = CONTROL_LEN_1;
	ctl_focusauto.pattr = (union ctlattr_t *)&focusautoitem;
#endif

}

void videocontrolifreq()
{
	uint8_t req = g_brequest;
	switch (g_wvalue_h) {
	case VC_REQUEST_ERROR_CODE_CONTROL:
		if (req == GET_CUR) {
			ep0_data_in(g_byvclasterror);
			send_respond();
			g_byvclasterror = VC_ERR_NOERROR;
			break;
		} else if (GET_INFO == req) {
			ep0_data_in(CONTROL_INFO_SP_GET);
			send_respond();
			break;
		} else {
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
		break;
	default:{
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
	}

	return;
}

void pantiltgetreqproc(int32_t sdwctlpan, int32_t sdwctltilt)
{
	uint16_t getlen;
	getlen = g_wlength;

	sdwctlpan *= 3600;
	sdwctltilt *= 3600;

	if (getlen >= 8) {
		int8_t *p = (int8_t *) &sdwctlpan;

		ep0_data_in(p[0]);
		ep0_data_in(p[1]);
		ep0_data_in(p[2]);
		ep0_data_in(p[3]);

		p = (int8_t *) &sdwctltilt;
		ep0_data_in(p[0]);
		ep0_data_in(p[1]);
		ep0_data_in(p[2]);
		ep0_data_in(p[3]);

		send_respond();
	} else {
		vcstall_epctl(VC_ERR_WRNGSTAT, 0);
		return;
	}

	return;
}

uint8_t pantiltsetreqcheckproc(struct ctl_t *pctl)
{
	uint16_t setlen;
	int32_t setpanvalue;
	int32_t settiltvalue;
	uint8_t i;
	uint8_t boutrange = 0;

	setlen = g_wlength;

	if (setlen != 8) {
		vcstall_epctl(VC_ERR_UNKNOWN, 0);
		return -1;
	}

	assign_byte((uint8_t *) &setpanvalue, 0, ep0_data_out());
	assign_byte((uint8_t *) &setpanvalue, 1, ep0_data_out());
	assign_byte((uint8_t *) &setpanvalue, 2, ep0_data_out());
	assign_byte((uint8_t *) &setpanvalue, 3, ep0_data_out());

	if ((setpanvalue > (int32_t) (panitem.max) * (int32_t) 3600)
	    || (setpanvalue < (int32_t) (panitem.min) * (int32_t) 3600)) {
		panitem.des = panitem.last;
		if (setpanvalue != g_dwpanfalsevalue_forxpbug) {
			DPRINT("out\n");
			boutrange = 1;
		}
	} else
		panitem.des = (short)(setpanvalue / 3600);

	assign_byte((uint8_t *) &settiltvalue, 0, ep0_data_out());
	assign_byte((uint8_t *) &settiltvalue, 1, ep0_data_out());
	assign_byte((uint8_t *) &settiltvalue, 2, ep0_data_out());
	assign_byte((uint8_t *) &settiltvalue, 3, ep0_data_out());

	if ((settiltvalue > (int32_t) (tiltitem.max) * (int32_t) 3600)
	    || (settiltvalue < (int32_t) (tiltitem.min) * (int32_t) 3600)) {
		DPRINT("out\n");
		tiltitem.des = tiltitem.last;
		boutrange = 1;
	} else
		tiltitem.des = (short)(settiltvalue / 3600);

	pctl->changeflag = CTL_CHNGFLG_NOTRDY;

	if (boutrange == 1)
		return -1;

	return 0;
}

uint8_t controlsetreqcheckproc(struct ctl_t *pctl)
{
	uint8_t len;
	uint16_t setlen;
	uint32_t setvalue32;
	uint16_t setvalue16;
	uint8_t setvalue8;
	uint8_t i;

	len = pctl->len;

	setlen = g_wlength;

	if ((setlen > len) || (setlen == 0)) {
		vcstall_epctl(VC_ERR_WRNGSTAT, 0);
		return -1;
	}

	if (len == 1) {
		setvalue8 = ep0_data_out();
	} else if (len == 2) {
		for (i = 0; i < setlen; i++)
			assign_byte((uint8_t *) &setvalue16,
				    i, ep0_data_out());
	} else {
		for (i = 0; i < setlen; i++)
			assign_byte((uint8_t *) &setvalue32,
				    i, ep0_data_out());
	}

	if (len == 1) {
		if (pctl->opmask & CONTROL_ATTR_SIGNED) {
			if ((*((int8_t *) &setvalue8) >
			     pctl->pattr->ctlitems8.max)
			    || (*((int8_t *) &setvalue8) <
				pctl->pattr->ctlitems8.min)) {
				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		} else {
			if ((setvalue8 > pctl->pattr->ctlitemu8.max)
			    || (setvalue8 < pctl->pattr->ctlitemu8.min)) {
				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		}

		pctl->pattr->ctlitemu8.des = setvalue8;
	} else if (len == 2) {
		if (pctl->opmask & CONTROL_ATTR_SIGNED) {
			if ((*((short *)&setvalue16) >
			     pctl->pattr->ctlitems16.max)
			    || (*((short *)&setvalue16) <
				pctl->pattr->ctlitems16.min)) {
				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		} else {
			if ((setvalue16 > pctl->pattr->ctlitemu16.max)
			    || (setvalue16 < pctl->pattr->ctlitemu16.min)) {

				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		}

		pctl->pattr->ctlitemu16.des = setvalue16;
	} else {
		if (pctl->opmask & CONTROL_ATTR_SIGNED) {
			if ((*((int32_t *) &setvalue32) >
			     pctl->pattr->ctlitems32.max)
			    || (*((int32_t *) &setvalue32) <
				pctl->pattr->ctlitems32.min)) {
				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		} else {
			if ((setvalue32 > pctl->pattr->ctlitemu32.max)
			    || (setvalue32 < pctl->pattr->ctlitemu32.min)) {
				vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
				return -1;
			}
		}

		pctl->pattr->ctlitemu32.des = setvalue32;
	}
	pctl->changeflag = CTL_CHNGFLG_NOTRDY;

	return 0;
}

void controlgetreqproc(struct ctl_t const *const pctl, uint8_t const req)
{
	uint8_t opmsk;
	uint8_t len;
	uint16_t getlen;
	uint8_t const *pch;
	uint8_t const *pmin;
	uint8_t const *pmax;
	uint8_t const *pres;
	uint8_t const *pdef;
	uint8_t const *plast;

	len = pctl->len;
	opmsk = pctl->opmask;

	if (len == 1) {
		pmin = (uint8_t *) &(pctl->pattr->ctlitemu8.min);
		pmax = (uint8_t *) &(pctl->pattr->ctlitemu8.max);
		pres = (uint8_t *) &(pctl->pattr->ctlitemu8.res);
		pdef = (uint8_t *) &(pctl->pattr->ctlitemu8.def);
		plast = (uint8_t *) &(pctl->pattr->ctlitemu8.last);
	} else if (len == 2) {
		pmin = (uint8_t *) &(pctl->pattr->ctlitemu16.min);
		pmax = (uint8_t *) &(pctl->pattr->ctlitemu16.max);
		pres = (uint8_t *) &(pctl->pattr->ctlitemu16.res);
		pdef = (uint8_t *) &(pctl->pattr->ctlitemu16.def);
		plast = (uint8_t *) &(pctl->pattr->ctlitemu16.last);
	} else if (len == 4) {
		pmin = (uint8_t *) &(pctl->pattr->ctlitemu32.min);
		pmax = (uint8_t *) &(pctl->pattr->ctlitemu32.max);
		pres = (uint8_t *) &(pctl->pattr->ctlitemu32.res);
		pdef = (uint8_t *) &(pctl->pattr->ctlitemu32.def);
		plast = (uint8_t *) &(pctl->pattr->ctlitemu32.last);
	}

	getlen = g_wlength;

	pch = pdef;

	switch (req) {
	case GET_INFO:{
			ep0_data_in(pctl->info);
			send_respond();
			return;
		}
	case GET_LEN:{
			if (opmsk & CONTROL_OP_GET_LEN) {
				ep0_data_in(pctl->len);

				if (getlen == 2)
					ep0_data_in(0);
				send_respond();
				return;
			} else {
				vcstall_epctl(VC_ERR_INVDREQ, 0);
				return;
			}
		}
	case GET_CUR:{
			if (opmsk & CONTROL_OP_GET_CUR) {
				if ((pctl->changeflag == CTL_CHNGFLG_RDY)) {

					pch = plast;
				} else {
					vcstall_epctl(VC_ERR_NOTRDY, 0);
					return;
				}
			} else {
				vcstall_epctl(VC_ERR_INVDREQ, 0);
				return;
			}
			break;
		}
	case GET_MIN:
		if (opmsk & CONTROL_OP_GET_MIN)
			pch = pmin;
		else {
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
		break;
	case GET_MAX:
		if (opmsk & CONTROL_OP_GET_MAX)
			pch = pmax;
		else {
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
		break;
	case GET_RES:
		if (opmsk & CONTROL_OP_GET_RES)
			pch = pres;
		else {
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
		break;
	case GET_DEF:
		if (opmsk & CONTROL_OP_GET_DEF)
			pch = pdef;
		else {
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
		break;
	default:{
			vcstall_epctl(VC_ERR_INVDREQ, 0);
			return;
		}
	}

	if (getlen >= len) {
		if (len == 1)
			ep0_data_in(pch[0]);
		else if (len == 2) {
			ep0_data_in(pch[0]);
			ep0_data_in(pch[1]);
		} else if (len == 4) {
			ep0_data_in(pch[0]);
			ep0_data_in(pch[1]);
			ep0_data_in(pch[2]);
			ep0_data_in(pch[3]);
		}
	} else {
		vcstall_epctl(VC_ERR_WRNGSTAT, 0);
		return;
	}

	send_respond();
	return;
}

void videoctlctreq()
{
	uint8_t req;
	uint8_t setvalue;
	uint8_t u8tmp;
	uint8_t i;
	req = g_brequest;

	switch (g_wvalue_h) {
	case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_EXPOSURE_TIME_ABS)
			    == CT_CTL_SEL_EXPOSURE_TIME_ABS) {
				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc
					    (&ctl_exposuretimeabsolut, req);
					break;
				case SET_CUR:
					if (g_setup_phrase == 1) {
						if (ctl_exposuretimeabsolut.info
						    & CONTROL_INFO_DIS_BY_AUTO) {
							vcstall_epctl
							    (VC_ERR_WRNGSTAT,
							     1);
							return;
						} else
							send_out_respond();
					} else {
						if (controlsetreqcheckproc
						    (&ctl_exposuretimeabsolut)
						    == 0) {
							ctl_exposuretimeabsolut.changeflag
							    = CTL_CHNGFLG_RDY;
							ctl_exposuretimeabsolut.pattr->
							    ctlitemu32.last =
							    ctl_exposuretimeabsolut.pattr->
							    ctlitemu32.des;
							set_video_ctrl
							    (capturefd,
							     V4L2_CID_EXPOSURE_ABSOLUTE,
							     ctl_exposuretimeabsolut.pattr->
							     ctlitemu32.last);

						}
					}
					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
	case CT_AE_MODE_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_AUTO_EXP_MODE) ==
			    CT_CTL_SEL_AUTO_EXP_MODE) {

				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_exposuretimeauto,
							  req);
					break;
				case SET_CUR:

					if (g_setup_phrase == 1) {
						send_out_respond();
						return;
					}

					setvalue = ep0_data_out();
					if ((setvalue ==
					     EXPOSURE_TIME_AUTO_MOD_APERTPRO)
					    || (setvalue ==
						EXPOSURE_TIME_AUTO_MOD_MANUAL)) {

						u8tmp =
						    (ctl_exposuretimeauto.pattr->
						     ctlitemu8.last);
						ctl_exposuretimeauto.
						    pattr->ctlitemu8.des =
						    setvalue;
						ctl_exposuretimeauto.
						    pattr->ctlitemu8.last =
						    setvalue;
						ctl_exposuretimeauto.changeflag
						    = CTL_CHNGFLG_RDY;

						if (u8tmp != setvalue) {

							if (setvalue ==
							    EXPOSURE_TIME_AUTO_MOD_APERTPRO) {
								ctl_exposuretimeabsolut.info
								    |=
								    CONTROL_INFO_DIS_BY_AUTO
								    |
								    CONTROL_INFO_SP_AUTOUPDATA;
							} else {
								ctl_exposuretimeabsolut.info
								    &=
								    ~
								    (CONTROL_INFO_DIS_BY_AUTO
								     |
								     CONTROL_INFO_SP_AUTOUPDATA);
							}

							set_video_ctrl
							    (capturefd,
							     V4L2_CID_EXPOSURE_AUTO,
							     ctl_exposuretimeauto.pattr->
							     ctlitemu8.last);

							ctl_exposuretimeabsolut.changeflag
							    = CTL_CHNGFLG_RDY;
							vcstatusintpacket_int0
							    (ENT_ID_CAMERA_IT,
							     CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
							     STS_PKT_ATTR_INFO_CHANGE,
							     &
							     (ctl_exposuretimeabsolut.info),
							     1);

						}
					} else {
						vcstall_epctl(VC_ERR_OUTOFRANGE,
							      0);
						return;
					}
					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
	case CT_AE_PRIORITY_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_AUTO_EXP_PRY) ==
			    CT_CTL_SEL_AUTO_EXP_PRY) {

				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_lowlightcomp,
							  req);
					break;
				case SET_CUR:
					if (g_setup_phrase == 1) {
						send_out_respond();
						return;
					}

					if (controlsetreqcheckproc
					    (&ctl_lowlightcomp) == 0) {

						ctl_lowlightcomp.changeflag =
						    CTL_CHNGFLG_RDY;
						ctl_lowlightcomp.
						    pattr->ctlitemu8.last =
						    ctl_lowlightcomp.
						    pattr->ctlitemu8.des;
						set_video_ctrl(capturefd,
							       V4L2_CID_EXPOSURE_AUTO_PRIORITY,
							       ctl_lowlightcomp.pattr->
							       ctlitemu8.last);
					}
					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
	case CT_PANTILT_ABSOLUTE_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_PANTILT_ABS) ==
			    CT_CTL_SEL_PANTILT_ABS) {
				switch (req) {
				case GET_INFO:
				case GET_LEN:
					controlgetreqproc(&ctl_pantilt, req);
					break;
				case GET_CUR:
					if ((g_brequesttype & 0x80)
						== 0x00) {
						if (g_wlength == 8) {
							for (i = 0; i < 4; i++) {
								assign_byte
								    ((uint8_t *)
								     &
								     g_dwpanfalsevalue_forxpbug,
								     i,
								     ep0_data_out
								     ());
							}
						}

						vcstall_epctl(VC_ERR_OUTOFRANGE,
							      0);
						return;
					}
					pantiltgetreqproc(panitem.last,
							  tiltitem.last);
					break;
				case GET_MAX:
					pantiltgetreqproc(panitem.max,
							  tiltitem.max);
					break;
				case GET_MIN:
					pantiltgetreqproc(panitem.min,
							  tiltitem.min);
					break;
				case GET_DEF:
					pantiltgetreqproc(panitem.def,
							  tiltitem.def);
					break;
				case GET_RES:
					pantiltgetreqproc(panitem.res,
							  tiltitem.res);
					break;
				case SET_CUR:
					if (g_setup_phrase == 1) {
						send_out_respond();
						return;
					}

					u8tmp =
					    pantiltsetreqcheckproc
					    (&ctl_pantilt);
					if (u8tmp == 0) {
						ctl_pantilt.changeflag =
						    CTL_CHNGFLG_RDY;
						panitem.last = panitem.des;
						tiltitem.last = tiltitem.des;

						set_video_ctrl(capturefd,
							       V4L2_CID_PAN_ABSOLUTE,
							       panitem.last *
							       3600);

						set_video_ctrl(capturefd,
							       V4L2_CID_TILT_ABSOLUTE,
							       tiltitem.last *
							       3600);

					} else {
						vcstall_epctl(VC_ERR_OUTOFRANGE,
							      0);

					}

					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
	case CT_ZOOM_ABSOLUTE_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_ZOOM_ABS) ==
			    CT_CTL_SEL_ZOOM_ABS) {
				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_zoom, req);
					break;
				case SET_CUR:
					if (g_setup_phrase == 1) {
						send_out_respond();
						return;
					}

					if (controlsetreqcheckproc(&ctl_zoom) ==
					    0) {
						ctl_zoom.changeflag =
						    CTL_CHNGFLG_RDY;
						ctl_zoom.pattr->
						    ctlitemu16.last =
						    ctl_zoom.pattr->
						    ctlitemu16.des;
						set_video_ctrl(capturefd,
							       V4L2_CID_ZOOM_ABSOLUTE,
							       ctl_zoom.pattr->
							       ctlitemu16.last);
					}
					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
	case CT_ROLL_ABSOLUTE_CONTROL:{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_ROLL_ABS) ==
			    CT_CTL_SEL_ROLL_ABS) {
				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_roll, req);
					break;
				case SET_CUR:
					if (g_setup_phrase == 1) {
						send_out_respond();
						return;
					}
					if (controlsetreqcheckproc(&ctl_roll) ==
					    0) {
						ctl_roll.changeflag =
						    CTL_CHNGFLG_RDY;
						ctl_roll.pattr->
						    ctlitems16.last =
						    ctl_roll.pattr->
						    ctlitems16.des;

						set_video_ctrl(capturefd,
							       V4L2_CID_ROLL_ABSOLUTE,
							       ctl_roll.pattr->
							       ctlitemu16.last);

					}
					break;
				default:{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}

				}
			} else {
				vcstall_epctl(VC_ERR_INVDCTL, 0);
				return;
			}
			break;
		}
#ifdef _AF_ENABLE_
	case CT_FOCUS_ABSOLUTE_CONTROL:
		{
			if ((g_wcamtrmctlsel & CT_CTL_SEL_FOCUS_ABS) ==
			    CT_CTL_SEL_FOCUS_ABS) {
				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_focusabsolut,
							  req);
					break;
				case SET_CUR:
					if (controlsetreqcheckproc
					    (&ctl_focusabsolut)) {
						ctl_focusabsolut.changeflag =
						    CTL_CHNGFLG_RDY;
						ctl_focusabsolut.pattr->
						    ctlitemu16.last =
						    ctl_focusabsolut.pattr->
						    ctlitemu16.des;
					}
					break;
				default:
					{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}
				}
			} else {
				vcstall_epctl(VC_ERR_INVDREQ, 0);
				return;
			}
			break;
		}
	case CT_FOCUS_AUTO_CONTROL:
		{
			if ((g_bycamtrmctlsel_ext & CT_CTL_SEL_FOCUS_AUTO) ==
			    CT_CTL_SEL_FOCUS_AUTO) {
				switch (req) {
				case GET_CUR:
				case GET_INFO:
				case GET_LEN:
				case GET_MAX:
				case GET_MIN:
				case GET_DEF:
				case GET_RES:
					controlgetreqproc(&ctl_focusauto, req);
					break;
				case SET_CUR:
					setvalue = ep0_data_out();
					if ((setvalue == 1)
					    || (setvalue == 0)) {
						u8tmp =
						    (ctl_focusauto.pattr->
						     ctlitemu8.last);
						ctl_focusauto.pattr->ctlitemu8.
						    des = setvalue;
						ctl_focusauto.pattr->ctlitemu8.
						    last = setvalue;
						ctl_focusauto.changeflag =
						    CTL_CHNGFLG_RDY;
						if (u8tmp != setvalue) {
							if (setvalue == 1) {
								ctl_focusabsolut.
								    info |=
								    CONTROL_INFO_DIS_BY_AUTO
								    |
								    CONTROL_INFO_SP_AUTOUPDATA;
							} else {
								ctl_focusabsolut.
								    info &=
								    ~
								    (CONTROL_INFO_DIS_BY_AUTO
								     |
								     CONTROL_INFO_SP_AUTOUPDATA);
							}
							ctl_focusabsolut.
							    changeflag =
							    CTL_CHNGFLG_RDY;
							vcstatusintpacket_int0
							    (ENT_ID_CAMERA_IT,
							     CT_FOCUS_ABSOLUTE_CONTROL,
							     STS_PKT_ATTR_INFO_CHANGE,
							     &(ctl_focusabsolut.
							       info), 1);
						}
					} else {
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}
					break;
				default:
					{
						vcstall_epctl(VC_ERR_INVDREQ,
							      0);
						return;
					}
				}
			} else {
				vcstall_epctl(VC_ERR_INVDREQ, 0);
				return;
			}
			break;
		}
#endif
	default:{
			DPRINT("invalid video ctl %d\n", g_wvalue_h);
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}

	}
	return;
}

void videoctlpureq()
{
	uint8_t req;
	uint8_t setvalue;
	uint8_t u8tmp;
	uint8_t bytmp2;
	req = g_brequest;

	switch (g_wvalue_h) {
	case PU_BRIGHTNESS_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_BRIGHTNESS)
		    == PU_CTL_SEL_BRIGHTNESS) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_brightness, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_brightness) ==
				    0) {
					ctl_brightness.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_brightness.pattr->ctlitems16.last =
					    ctl_brightness.pattr->
					    ctlitems16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_BRIGHTNESS,
						       ctl_brightness.
						       pattr->ctlitems16.last);

				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_CONTRAST_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_CONTRAST) ==
		    PU_CTL_SEL_CONTRAST) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_contrast, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_contrast) == 0) {
					ctl_contrast.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_contrast.pattr->ctlitemu16.last =
					    ctl_contrast.pattr->ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_CONTRAST,
						       ctl_contrast.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_HUE_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_HUE) == PU_CTL_SEL_HUE) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_hue, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_hue) == 0) {
					ctl_hue.changeflag = CTL_CHNGFLG_RDY;
					ctl_hue.pattr->ctlitems16.last =
					    ctl_hue.pattr->ctlitems16.des;
					set_video_ctrl(capturefd, V4L2_CID_HUE,
						       ctl_hue.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_SATURATION_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_SATURATION) ==
		    PU_CTL_SEL_SATURATION) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_saturation, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_saturation) ==
				    0) {
					ctl_saturation.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_saturation.pattr->ctlitemu16.last =
					    ctl_saturation.pattr->
					    ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_SATURATION,
						       ctl_saturation.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_SHARPNESS_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_SHARPNESS)
		    == PU_CTL_SEL_SHARPNESS) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_sharpness, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_sharpness) == 0) {
					ctl_sharpness.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_sharpness.pattr->ctlitemu16.last =
					    ctl_sharpness.pattr->ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_SHARPNESS,
						       ctl_sharpness.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_GAMMA_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_GAMMA)
		    == PU_CTL_SEL_GAMMA) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_gamma, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_gamma) == 0) {
					ctl_gamma.changeflag = CTL_CHNGFLG_RDY;
					ctl_gamma.pattr->ctlitemu16.last =
					    ctl_gamma.pattr->ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_GAMMA,
						       ctl_gamma.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_BACKLIGHT_COMPENSATION_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_BACKLIGHT_COMPENSATION)
		    == PU_CTL_SEL_BACKLIGHT_COMPENSATION) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_backlightcomp, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}

				if (controlsetreqcheckproc(&ctl_backlightcomp)
				    == 0) {
					ctl_backlightcomp.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_backlightcomp.pattr->
					    ctlitemu16.last =
					    ctl_backlightcomp.pattr->
					    ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_BACKLIGHT_COMPENSATION,
						       ctl_backlightcomp.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_GAIN_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_GAIN) == PU_CTL_SEL_GAIN) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_gain, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_gain) == 0) {
					ctl_gain.changeflag = CTL_CHNGFLG_RDY;
					ctl_gain.pattr->ctlitemu16.last =
					    ctl_gain.pattr->ctlitemu16.des;
					set_video_ctrl(capturefd, V4L2_CID_GAIN,
						       ctl_gain.
						       pattr->ctlitems16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_POWER_LINE_FREQUENCY_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_POWER_LINE_FREQUENCY)
		    == PU_CTL_SEL_POWER_LINE_FREQUENCY) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_pwrlinefreq, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				if (controlsetreqcheckproc(&ctl_pwrlinefreq) ==
				    0) {
					ctl_pwrlinefreq.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_pwrlinefreq.pattr->ctlitemu8.last =
					    ctl_pwrlinefreq.pattr->
					    ctlitemu8.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_POWER_LINE_FREQUENCY,
						       ctl_pwrlinefreq.
						       pattr->ctlitemu8.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
		if ((g_wprocunitctlsel & PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE)
		    == PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_whitebalancetemp, req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					if (ctl_whitebalancetemp.info &
					    CONTROL_INFO_DIS_BY_AUTO) {
						vcstall_epctl(VC_ERR_WRNGSTAT,
							      1);
						return;
					} else
						send_out_respond();
					return;
				}
				if (controlsetreqcheckproc
				    (&ctl_whitebalancetemp) == 0) {
					ctl_whitebalancetemp.changeflag =
					    CTL_CHNGFLG_RDY;
					ctl_whitebalancetemp.pattr->
					    ctlitemu16.last =
					    ctl_whitebalancetemp.
					    pattr->ctlitemu16.des;
					set_video_ctrl(capturefd,
						       V4L2_CID_WHITE_BALANCE_TEMPERATURE,
						       ctl_whitebalancetemp.pattr->
						       ctlitemu16.last);
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
		if ((g_wprocunitctlsel &
		     PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE_AUTO)
		    == PU_CTL_SEL_WHITE_BALANCE_TEMPERATURE_AUTO) {
			switch (req) {
			case GET_CUR:
			case GET_INFO:
			case GET_LEN:
			case GET_MAX:
			case GET_MIN:
			case GET_DEF:
			case GET_RES:
				controlgetreqproc(&ctl_whitebalancetempauto,
						  req);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				setvalue = ep0_data_out();
				if ((setvalue == 1) || (setvalue == 0)) {
					u8tmp =
					    ctl_whitebalancetempauto.
					    pattr->ctlitemu8.last;

					ctl_whitebalancetempauto.
					    pattr->ctlitemu8.des = setvalue;
					ctl_whitebalancetempauto.
					    pattr->ctlitemu8.last = setvalue;

					ctl_whitebalancetempauto.changeflag
					    = CTL_CHNGFLG_RDY;

					if (u8tmp != setvalue) {
						if (setvalue == 1) {
							ctl_whitebalancetemp.info
							    |=
							    CONTROL_INFO_DIS_BY_AUTO;
							bytmp2 = 1;
						} else {
							ctl_whitebalancetemp.info
							    &=
							    ~
							    (CONTROL_INFO_DIS_BY_AUTO);
							bytmp2 = 0;
						}

						set_video_ctrl(capturefd,
							       V4L2_CID_AUTO_WHITE_BALANCE,
							       ctl_whitebalancetempauto.pattr->
							       ctlitemu8.last);

						ctl_whitebalancetemp.changeflag
						    = CTL_CHNGFLG_RDY;
						vcstatusintpacket_int0
						    (ENT_ID_PROCESSING_UNIT,
						     PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
						     STS_PKT_ATTR_INFO_CHANGE,
						     &
						     (ctl_whitebalancetemp.info),
						     1);

					} else {
					}

				} else {
					vcstall_epctl(VC_ERR_OUTOFRANGE, 0);
					return;
				}
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
		} else {
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
		break;
	default:{
			DPRINT("invalid pu ctl %d\n", g_wvalue_h);
			vcstall_epctl(VC_ERR_INVDCTL, 0);
			return;
		}
	}
	return;
}

vcmdsts_t cmddbgsdataout(uint8_t bysubcmd, uint16_t waddr, uint8_t len)
{
	uint8_t i;
	uint16_t wvalue;
	uint32_t dwvalue = 0;
	uint8_t bytemp1, bytemp2;
	uint8_t abyi2cdatabuf[8];

	switch (bysubcmd) {
	case VSCMD_XMEM_W:{
			int8_t *buf = malloc(len);
			for (i = 0; i < len; i++)
				buf[i] = ep0_data_out();

			rts_write_xmem(capturefd, waddr, len, buf);
			free(buf);
			break;
		}
	default:{
			return (uint8_t) VCMD_STS_UNKNOWN_SCMD;
		}
	}
	return (uint8_t) VCMD_STS_RET_NO_ERR;
}

static void getdevstatus(uint8_t bysection, uint8_t bylen)
{
	uint8_t i;

	switch (bysection) {
	case 0:
		ep0_data_in(_RT_VID_L_);
		ep0_data_in(_RT_VID_H_);
		ep0_data_in(_PID_L_);
		ep0_data_in(_PID_H_);
		ep0_data_in(_FW_SUB_VER_);
		ep0_data_in(_FW_MAIN_VER_);
		ep0_data_in(g_byhwid1);
		ep0_data_in(g_byhwid2);
		break;
	case 1:
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(0);
		break;
	case 2:
	case 11:
		for (i = 0; i < 8; i++)
			ep0_data_in(fwcompdatestring[i]);
		break;
	case 4:
		ep0_data_in(0);
		ep0_data_in(0);
		ep0_data_in(AP_MAIN_VERSION);
		ep0_data_in(AP_SUB_VERSION);
		for (i = 4; i < 8; i++)
			ep0_data_in(0);

		break;
	case 8:
		for (i = 0; i < 8; i++)
			ep0_data_in(0);
		break;
	default:
		for (i = 0; i < 8; i++)
			ep0_data_in(DUMMY_BYTE);
		break;
	}

	for (i = 0; i < (bylen - 8); i++)
		ep0_data_in(DUMMY_BYTE);
}

vcmdsts_t cmddbgsdatain(uint8_t bysubcmd, uint16_t waddr, uint8_t len)
{
	uint8_t i;
	uint16_t wvalue;
	uint32_t dwvalue;
	uint8_t bytemp1;
	uint8_t abyi2cdatabuf[8];

	switch (bysubcmd) {
	case VSCMD_CODE_R:
	case VSCMD_IMEM_R:
	case VSCMD_XMEM_R:{
			for (i = 0; i < len; i++) {
				if (bysubcmd == VSCMD_CODE_R)
					ep0_data_in(0);
				else if (bysubcmd == VSCMD_IMEM_R)
					ep0_data_in(0);
				else if (bysubcmd == VSCMD_XMEM_R) {
					int8_t *buf = malloc(len);
					if (rts_read_xmem
					    (capturefd, waddr, len, buf))
						DPRINT("rts_read_xmem error");
					for (i = 0; i < len; i++)
						ep0_data_in(buf[i]);
					free(buf);

				}
			}
			break;
		}
	case VSCMD_DEV_STS_R:{
			getdevstatus((uint8_t) waddr, len);
			break;
		}
	case VSCMD_DEV_QUALIFICATION_GET:{
			for (i = 0; i < 8; i++)
				ep0_data_in(qualificationstring[i]);
			break;
		}
	default:
		return (uint8_t) VCMD_STS_UNKNOWN_SCMD;
	}
	return (uint8_t) VCMD_STS_RET_NO_ERR;
}

vcmdsts_t cmddbgsdbgcmd(uint8_t bysubcmd, uint16_t wlen, uint8_t bycmdflag)
{
	vcmdsts_t ret = VCMD_STS_CMD;
	switch (bysubcmd) {
	case VSCMD_CODE_R:
	case VSCMD_IMEM_R:
	case VSCMD_XMEM_R:
	case VSCMD_BAR_INFO_READ:
		if (wlen == 0)
			ret = (uint8_t) VCMD_STS_ERR_LEN;
		else
			ret = (uint8_t) VCMD_STS_DATA_IN;
		break;
	case VSCMD_DEV_STS_R:
	case VSCMD_DEV_QUALIFICATION_GET:
		if (wlen < 8)
			ret = (uint8_t) VCMD_STS_ERR_LEN;
		else
			ret = (uint8_t) VCMD_STS_DATA_IN;
		break;
	case VSCMD_XMEM_W:
		if (wlen == 0)
			ret = (uint8_t) VCMD_STS_ERR_LEN;
		else
			ret = (uint8_t) VCMD_STS_DATA_OUT;
		break;
	default:
		ret = (uint8_t) VCMD_STS_UNKNOWN_SCMD;
		break;
	}
	return ret;
}

void vendorcmdproc(uint8_t cs, uint8_t req, uint16_t reqlen)
{
	static uint16_t wlen;
	static uint8_t bycmd;
	static uint8_t bysubcmd;
	static uint32_t dwaddr;
	static vcmdsts_t cmdstatus = VCMD_STS_CMD;
	uint8_t bytmp;
	uint16_t wtemp, wtemp2;
	vcmdsts_t bytmp2;
	uint16_t i;
	uint16_t wep0buffersize;

	wep0buffersize = 64;

	if (cs == EXU1_CMD_STS) {
		if (req == GET_CUR) {
			int8_t *p = (int8_t *) &g_wdatainoutctlsize;
			ep0_data_in((uint8_t) cmdstatus);
			ep0_data_in(p[0]);
			ep0_data_in(p[1]);
			for (bytmp = 3; bytmp < 8; bytmp++)
				ep0_data_in(DUMMY_BYTE);
			send_respond();
		} else {

			bycmd = ep0_data_out();
			bysubcmd = ep0_data_out();

			if (bycmd == VCMD_RST) {
				cmdstatus = VCMD_STS_CMD;
				return;
			}

			if (cmdstatus != VCMD_STS_CMD) {
				vcstall_epctl(VC_ERR_WRNGSTAT, 0);
				cmdstatus = VCMD_STS_NOT_FINISH;
				return;
			}

			dwaddr = ep0_data_out();
			bytmp = ep0_data_out();
			dwaddr |= ((uint32_t) bytmp) << 8;
			wlen = ep0_data_out();
			bytmp = ep0_data_out();
			wlen |= ((uint16_t) bytmp) << 8;
			bytmp = ep0_data_out();
			dwaddr |= ((uint32_t) bytmp) << 16;
			bytmp = ep0_data_out();
			dwaddr |= ((uint32_t) bytmp) << 24;

			switch (bycmd) {
			case VCMD_DBG:
				if (bycmd == VCMD_DBG)
					cmdstatus =
					    cmddbgsdbgcmd(bysubcmd, wlen,
							  (uint8_t)
							  dwaddr);

				if ((cmdstatus == VCMD_STS_CMD)
				    || (cmdstatus == VCMD_STS_DATA_IN)
				    || (cmdstatus == VCMD_STS_DATA_OUT))
					break;
				else {
					vcstall_epctl(VC_ERR_WRNGSTAT, 0);
					return;
				}
			default:{
					vcstall_epctl(VC_ERR_WRNGSTAT, 0);
					cmdstatus = VCMD_STS_UNKNOWN_CMD;
					return;
				}
			}
		}
	} else {
		if (wlen <= reqlen)
			wtemp = wlen;
		else
			wtemp = reqlen;

		if (req == GET_CUR) {
			if (cmdstatus != VCMD_STS_DATA_IN) {
				vcstall_epctl(VC_ERR_WRNGSTAT, 0);
				cmdstatus = VCMD_STS_ERR_DIR;
				return;
			}

			wtemp2 = wtemp;
			while (1) {
				bytmp =
				    wtemp2 >
				    wep0buffersize ? wep0buffersize : wtemp2;
				switch (bycmd) {
				case VCMD_DBG:{
						if ((bysubcmd & SCMD_DATA_VLD)
						    && (SCMD_DIR_IN ==
							(bysubcmd &
							 SCMD_DIR_MSK))) {
							if (bycmd == VCMD_DBG)
								bytmp2 =
								    cmddbgsdatain
								    (bysubcmd,
								     (unsigned
								      short)
								     dwaddr,
								     bytmp);

							if ((uint8_t)
							    VCMD_STS_RET_NO_ERR
							    != bytmp2) {
								vcstall_epctl
								    (VC_ERR_WRNGSTAT,
								     0);
								cmdstatus =
								    bytmp2;
								return;
							}
						} else {
							vcstall_epctl
							    (VC_ERR_WRNGSTAT,
							     0);
							cmdstatus =
							    VCMD_STS_ERR_DIR;
							return;
						}
						break;
					}

				default:{
						vcstall_epctl(VC_ERR_WRNGSTAT,
							      0);
						cmdstatus =
						    VCMD_STS_UNKNOWN_CMD;
						return;
					}
				}

				wtemp2 -= bytmp;
				dwaddr += bytmp;
				if (wtemp2 == 0)
					break;
			}

			for (i = wtemp; i < reqlen; i++)
				ep0_data_in(DUMMY_BYTE);

			send_respond();
		} else {

			if (cmdstatus != VCMD_STS_DATA_OUT) {
				vcstall_epctl(VC_ERR_WRNGSTAT, 0);
				cmdstatus = VCMD_STS_ERR_DIR;
				return;
			}

			wtemp2 = wtemp;
			while (1) {
				bytmp =
				    wtemp2 >
				    wep0buffersize ? wep0buffersize : wtemp2;

				switch (bycmd) {
				case VCMD_DBG:{
						if ((bysubcmd & SCMD_DATA_VLD)
						    && (SCMD_DIR_OUT ==
							(bysubcmd &
							 SCMD_DIR_MSK))) {
							if (bycmd == VCMD_DBG) {
								bytmp2 =
								    cmddbgsdataout
								    (bysubcmd,
								     (unsigned
								      short)
								     dwaddr,
								     bytmp);
							}
							if ((uint8_t)
							    VCMD_STS_RET_NO_ERR
							    != bytmp2) {
								vcstall_epctl
								    (VC_ERR_WRNGSTAT,
								     0);
								cmdstatus =
								    bytmp2;
								return;
							}
						} else {
							vcstall_epctl
							    (VC_ERR_WRNGSTAT,
							     0);
							cmdstatus = (uint8_t)
							    VCMD_STS_ERR_DIR;
							return;
						}
						break;
					}
				default:{
						vcstall_epctl(VC_ERR_WRNGSTAT,
							      0);
						cmdstatus = (uint8_t)
						    VCMD_STS_UNKNOWN_CMD;
						return;
					}
				}

				wtemp2 -= bytmp;
				dwaddr += bytmp;
				if (wtemp2 == 0)
					break;
			}
		}
		wlen -= wtemp;

		if (wlen == 0)
			cmdstatus = (uint8_t) VCMD_STS_CMD;
	}
	return;
}

void videoctlexudbgreq()
{
	uint8_t req, cs;
	uint16_t wreqlen;
	uint8_t i;

	req = g_brequest;
	cs = g_wvalue_h;
	wreqlen = g_wlength;

	switch (cs) {
	case EXU1_CMD_STS:
	case EXU1_DATA_INOUT:{
			switch (req) {
			case GET_MIN:
			case GET_DEF:
				if (cs == EXU1_CMD_STS)
					for (i = 0; i < MAX_RWFLASH_SIZE; i++)
						ep0_data_in(0);
				else
					for (i = 0; i < wreqlen; i++)
						ep0_data_in(0);

				send_respond();
				break;
			case GET_MAX:
				if (cs == EXU1_CMD_STS)
					for (i = 0; i < MAX_RWFLASH_SIZE; i++)
						ep0_data_in(0xff);
				else
					for (i = 0; i < wreqlen; i++)
						ep0_data_in(0xff);

				send_respond();
				break;
			case GET_RES:
				ep0_data_in(0x1);
				if (cs == EXU1_CMD_STS)
					for (i = 0; i < MAX_RWFLASH_SIZE - 1;
					     i++)
						ep0_data_in(0);
				else
					for (i = 0; i < wreqlen - 1; i++)
						ep0_data_in(0);

				send_respond();
				break;
			case GET_INFO:
				ep0_data_in(CONTROL_INFO_SP_GET |
					    CONTROL_INFO_SP_SET);
				send_respond();
				break;
			case GET_LEN:
				if (cs == EXU1_CMD_STS) {
					ep0_data_in(MAX_RWFLASH_SIZE);
					ep0_data_in(0);
				} else {
					int8_t *p =
					    (int8_t *) &g_wdatainoutctlsize;
					ep0_data_in(p[0]);
					ep0_data_in(p[1]);
				}
				send_respond();
				break;
			case GET_CUR:
				vendorcmdproc(cs, GET_CUR, wreqlen);
				break;
			case SET_CUR:
				if (g_setup_phrase == 1) {
					send_out_respond();
					return;
				}
				vendorcmdproc(cs, SET_CUR, wreqlen);
				break;
			default:{
					vcstall_epctl(VC_ERR_INVDREQ, 0);
					return;
				}
			}
			break;
		}
	default:{
			vcstall_epctl(VC_ERR_INVDCTL, 0);

			return;
		}
	}
}

void videocontrolreq()
{
	switch (g_windex_h) {
	case 0x00:
		videocontrolifreq();
		break;
	case ENT_ID_CAMERA_IT:
		DPRINT("ENT_ID_CAMERA_IT\n");
		videoctlctreq();
		break;
	case ENT_ID_PROCESSING_UNIT:
		DPRINT("ENT_ID_PROCESSING_UNIT\n");
		videoctlpureq();
		break;
	case ENT_ID_EXTENSION_UNIT_DBG:
		DPRINT("ENT_ID_EXTENSION_UNIT_DBG\n");
		videoctlexudbgreq();
		break;
	case ENT_ID_OUTPUT_TRM:
		DPRINT("ENT_ID_OUTPUT_TRM\n");
		vcstall_epctl(VC_ERR_INVDCTL, 0);
		break;
	default:
		DPRINT("videocontrolreq not\n");
		vcstall_epctl(VC_ERR_INVDUNIT, 0);
		break;
	}
	return;
}

void usbclsreqproc()
{
	switch (g_brequest) {
	case SET_CUR:
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_LEN:
	case GET_INFO:
	case GET_DEF:
		videoclsreqproc();
		break;
	default:
		stall_epctl(0);
		break;
	}
	return;
}

void usbvendorreqproc()
{
	int32_t i;
	uint8_t byrequestcmd, bysubpara;

	switch (g_brequest) {
	case USB_VDREQUEST_GET_STATUS:
		ep0_data_in(0);
		send_respond();
		break;
	case USB_VDREQUEST_DEV_STS_R:
		byrequestcmd = g_brequest;
		bysubpara = g_wvalue_l;

		switch (bysubpara) {
		case 0:
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			ep0_data_in(0);
			send_respond();
			break;
		case 3:
			for (i = 0; i < 8; i++)
				ep0_data_in(0);
			send_respond();

			break;
		case 8:
			for (i = 0; i < 8; i++)
				ep0_data_in(0);
			send_respond();

			break;
		default:
			for (i = 0; i < 8; i++)
				ep0_data_in(0);
			send_respond();

			break;
		}
		break;

	default:
		stall_epctl(0);
		break;
	}
}

void usbprosetuppkt(void)
{
	switch (g_brequesttype & REQUEST_TYPE) {
	case STANDARD_REQUEST:
		stall_epctl(0);
		errno_exit("should not run here");
		break;
	case CLASS_REQUEST:
		usbclsreqproc();
		break;
	case VENDOR_REQUEST:
		usbvendorreqproc();
		break;
	default:
		stall_epctl(0);
		break;
	}
	return;
}

void setup_process()
{
	if ((g_brequesttype == 0x82)
	    && (g_brequest == 0x0c)) {
		stall_epctl(0);
		return;
	}

	usbprosetuppkt();
	return;
}

int32_t get_capture_image(struct v4l2_buffer *v4l2buf)
{

	fd_set fds;
	int32_t r;

	DPRINT("get capture image\n");

	FD_ZERO(&fds);
	FD_SET(capturefd, &fds);

_GET_IMAGE:
	DPRINT("wait frame buffer\n");

	r = select(capturefd + 1, &fds, NULL, NULL, NULL);

	DPRINT("exit waiting\n");
	if (-1 == r) {
		if (EINTR == errno)
			goto _GET_IMAGE;
		errno_exit("select");
	}

	if (0 == r)
		exit(EXIT_FAILURE);

	uint32_t i;

	CLEAR(*v4l2buf);

	v4l2buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2buf->memory = V4L2_MEMORY_MMAP;

_GET_IMAGE_EAGAIN:
	if (-1 == xioctl(capturefd, VIDIOC_DQBUF, v4l2buf)) {
		switch (errno) {
		case EAGAIN:
			usleep(1000);
			DPRINT("VIDIOC_DQBUF EAGAIN\n");
			goto _GET_IMAGE_EAGAIN;

		case EIO:
			/* Could ignore EIO, see spec. */
			/* fall through */
			errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(v4l2buf->index < BUFFER_COUNT);

	DPRINT("Got frame buffer %d\n", v4l2buf->index);

	return 0;
}

void put_capture_image(struct v4l2_buffer *v4l2buf)
{
	DPRINT("put capture image\n");

	if (-1 == xioctl(capturefd, VIDIOC_QBUF, v4l2buf))
		errno_exit("VIDIOC_QBUF");
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;
	DPRINT("stop capturing\n");

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(capturefd, VIDIOC_STREAMOFF, &type))
		errno_exit("VIDIOC_STREAMOFF");
}

static void start_capturing(void)
{
	uint32_t i;
	enum v4l2_buf_type type;
	DPRINT("start capturing\n");

	for (i = 0; i < BUFFER_COUNT; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(capturefd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(capturefd, VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");
}

static void uninit_usb_device(void)
{
#ifdef LOAD_FROM_IMAGE_FILE

	uint32_t i;
	for (i = 0; i < BUFFER_COUNT; ++i)
		free(usbbuffers[i].start);
	free(usbbuffers);
#endif
}

unsigned long loadimage(const int8_t *path, int8_t *buf, int32_t buflen)
{
	struct stat statbuff;
	if (stat(path, &statbuff) < 0) {
		memset(buf, 0xff, buflen);
		DPRINT("can't find image file %s\n", path);
		return 0;
	}

	int32_t filelen = statbuff.st_size - 128;

	int32_t fd;
	fd = open(path, O_RDWR, 0);
	if (-1 == fd) {
		DPRINT("cannot open image.cap: %d, %s\n",
		       errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	int32_t len = filelen > buflen ? buflen : filelen;

	DPRINT("load image file %s %d %d %d\n", path, len, buflen, filelen);

	if (pread(fd, buf, len, 128) != len) {
		DPRINT("read image.cap errors: %d, %s\n",
		       errno, strerror(errno));
	}

	close(fd);
	return 0;
}

void init_stuff(uint32_t buffer_size)
{
	int32_t i;
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(usbfd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			DPRINT("video device does not support "
			       "user pointer i/o\n");
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
#ifdef LOAD_FROM_IMAGE_FILE
	for (i = 0; i < BUFFER_COUNT; ++i) {
		usbbuffers[i].length = buffer_size;
		usbbuffers[i].start = malloc(buffer_size);
		if (!usbbuffers[i].start) {
			DPRINT("out of memory\n");
			exit(EXIT_FAILURE);
		}
		loadimage("/bin/image.cap", usbbuffers[i].start,
			  usbbuffers[i].length);
	}
#endif
}

#ifdef LOAD_FROM_IMAGE_FILE

void fill_stuff()
{
	uint32_t i;
	enum v4l2_buf_type type;

	sleep(2);

	DPRINT("fill init stuff\n");

	for (i = 0; i < BUFFER_COUNT; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.m.userptr = (unsigned long)usbbuffers[i].start;
		buf.length = usbbuffers[i].length;
		buf.bytesused = usbbuffers[i].length;

_USB_QBUF_EAGAIN:
		if (-1 == xioctl(usbfd, VIDIOC_QBUF, &buf))
			switch (errno) {
			case EAGAIN:
				usleep(1000);
				DPRINT("usb qbuf eagain\n");
				goto _USB_QBUF_EAGAIN;

			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
				errno_exit("VIDIOC_QBUF");
			}

	}
}

#endif

static void close_usb_device(void)
{
	uninit_usb_device();

	if (-1 == close(usbfd))
		errno_exit("close");

	usbfd = -1;
}

static void open_usb_device(void)
{
	struct stat st;
	struct v4l2_format fmt;

	if (-1 == stat(usb_dev, &st)) {
		DPRINT("cannot identify '%s': %d, %s\n",
		       usb_dev, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	usbfd = open(usb_dev, O_RDWR | O_NONBLOCK, 0);
	if (-1 == usbfd) {
		DPRINT("cannot open '%s': %d, %s\n",
		       usb_dev, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct v4l2_capability cap;
	if (-1 == xioctl(usbfd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			DPRINT("%s is no v4l2 device\n", usb_dev);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		DPRINT("%s is no video output device\n", usb_dev);
		exit(EXIT_FAILURE);
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DPRINT("%s does not support streaming i/o\n", usb_dev);
		exit(EXIT_FAILURE);
	}

	struct v4l2_event_subscription ves;
	memset(&ves, 0, sizeof(ves));
	ves.type = UVC_EVENT_STREAMON;
	if (-1 == xioctl(usbfd, VIDIOC_SUBSCRIBE_EVENT, &ves))
		errno_exit("subscribe UVC_EVENT_STREAMON error");

	ves.type = UVC_EVENT_STREAMOFF;
	if (-1 == xioctl(usbfd, VIDIOC_SUBSCRIBE_EVENT, &ves))
		errno_exit("subscribe UVC_EVENT_STREAMOFF error");

	ves.type = UVC_EVENT_SETUP;
	if (-1 == xioctl(usbfd, VIDIOC_SUBSCRIBE_EVENT, &ves))
		errno_exit("subscribe UVC_EVENT_SETUP error");

	ves.type = UVC_EVENT_DATA;
	if (-1 == xioctl(usbfd, VIDIOC_SUBSCRIBE_EVENT, &ves))
		errno_exit("subscribe UVC_EVENT_DATA error");

	ves.type = UVC_EVENT_DISCONNECT;
	if (-1 == xioctl(usbfd, VIDIOC_SUBSCRIBE_EVENT, &ves))
		errno_exit("subscribe UVC_EVENT_DATA error");

	/* Note VIDIOC_S_FMT may change width and height. */
#ifdef LOAD_FROM_IMAGE_FILE
	usbbuffers = calloc(4, sizeof(*usbbuffers));
	if (!usbbuffers) {
		DPRINT("out of memory\n");
		exit(EXIT_FAILURE);
	}
#endif

}

static void uninit_capture_device(void)
{
	uint32_t i;

	for (i = 0; i < BUFFER_COUNT; ++i)
		if (-1 ==
		    munmap(capturebuffers[i].start, capturebuffers[0].length))
			DPRINT("munmap error %d, %s %d %x %d\n", errno, strerror(errno),
				i, capturebuffers[i].start, capturebuffers[0].length);

	free(capturebuffers);
}

static void init_capture_buffers(void)
{
	struct v4l2_requestbuffers req;
	int32_t i;
	DPRINT("init_capture_buffers\n");

	CLEAR(req);

	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(capturefd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			DPRINT("%s does not support memory mapping\n",
				capture_dev);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		DPRINT("Insufficient buffer memory on %s\n", capture_dev);
		exit(EXIT_FAILURE);
	}

	capturebuffers = calloc(req.count, sizeof(*capturebuffers));

	if (!capturebuffers) {
		DPRINT("Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < req.count; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(capturefd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		capturebuffers[i].length = buf.length;
		capturebuffers[i].start = mmap(NULL /* start anywhere */ ,
					       capturebuffers[i].length,
					       PROT_READ | PROT_WRITE
					       /* required */ ,
					       MAP_SHARED /* recommended */ ,
					       capturefd, buf.m.offset);

		DPRINT("capture buf:%d %x %d\n", i, capturebuffers[i].start,
		       capturebuffers[i].length);

		if (MAP_FAILED == capturebuffers[i].start)
			errno_exit("mmap");
	}
}

static int open_capture_device(int type)
{
	struct stat st;

	if (type == V4L2_PIX_FMT_NV12
	    || type == V4L2_PIX_FMT_H264) {

		DPRINT("open capture device %s\n", capture_dev_yuv420);

		if (-1 == stat(capture_dev_yuv420, &st)) {
			DPRINT("Cannot identify '%s': %d, %s\n",
			       capture_dev_yuv420, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (!S_ISCHR(st.st_mode)) {
			DPRINT("%s is no device\n", capture_dev_yuv420);
			exit(EXIT_FAILURE);
		}

		capturefd_yuv420 = open(capture_dev_yuv420, O_RDWR | O_NONBLOCK, 0);

		if (-1 == capturefd_yuv420) {
			DPRINT("Cannot open '%s': %d, %s\n",
			       capture_dev_yuv420, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		capturefd = capturefd_yuv420;
		strcpy(capture_dev, capture_dev_yuv420);
	} else if (type == V4L2_PIX_FMT_MJPEG) {
		DPRINT("open capture device %s\n", capture_dev_mjpeg);

		if (-1 == stat(capture_dev_mjpeg, &st)) {
			DPRINT("Cannot identify '%s': %d, %s\n",
			       capture_dev_mjpeg, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (!S_ISCHR(st.st_mode)) {
			DPRINT("%s is no device\n", capture_dev_mjpeg);
			exit(EXIT_FAILURE);
		}

		capturefd_m = open(capture_dev_mjpeg, O_RDWR | O_NONBLOCK, 0);

		if (-1 == capturefd_m) {
			DPRINT("Cannot open '%s': %d, %s\n",
			       capture_dev_mjpeg, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
		capturefd = capturefd_m;
		strcpy(capture_dev, capture_dev_mjpeg);
	} else {

		DPRINT("not support format %d %d %c%c%c%c\n",
		       ((int8_t *) &(type))[0],
		       ((int8_t *) &(type))[1],
		       ((int8_t *) &(type))[2],
		       ((int8_t *) &(type))[3]);

		return -1;
	}

	return 0;

}

static void init_capture_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	uint32_t min;
	int32_t ret;

	DPRINT("init_capture_device\n");

	if (-1 == xioctl(capturefd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			DPRINT("%s is no V4L2 device\n", capture_dev);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		DPRINT("%s is no video capture device\n", capture_dev);
		exit(EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DPRINT("%s does not support streaming i/o\n", capture_dev);
		exit(EXIT_FAILURE);
	}

	/* Select video input, video standard and tune here. */
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(capturefd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */

		if (-1 == xioctl(capturefd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = commitfs.width;
	fmt.fmt.pix.height = commitfs.height;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (commitfs.type == V4L2_PIX_FMT_H264)
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	else
		fmt.fmt.pix.pixelformat = commitfs.type;


	ret = xioctl(capturefd, VIDIOC_S_FMT, &fmt);
	if (ret)
		errno_exit("capture VIDIOC_S_FMT");

	struct v4l2_streamparm Stream_Parm;
	memset(&Stream_Parm, 0, sizeof(struct v4l2_streamparm));
	Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	Stream_Parm.parm.capture.timeperframe.denominator =
	    10000000 / g_vscommit.dwframeinterval;
	Stream_Parm.parm.capture.timeperframe.numerator = 1;


	DPRINT("set capture format %d %d %c%c%c%c\n", fmt.fmt.pix.width, fmt.fmt.pix.height,
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[0],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[1],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[2],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[3]);

	DPRINT("fps %d/%d\n",
	       Stream_Parm.parm.capture.timeperframe.numerator,
	       Stream_Parm.parm.capture.timeperframe.denominator);

	if (xioctl(capturefd, VIDIOC_S_PARM, &Stream_Parm))
		errno_exit("VIDIOC_S_PARM");

	if (commitfs.type == V4L2_PIX_FMT_H264) {
		DPRINT("init h264 encoder\n");
		uvc_h264enc_init(commitfs.width, commitfs.height, Stream_Parm.parm.capture.timeperframe.denominator);
	}

}

static void close_capture_device(void)
{
	DPRINT("close capture device\n");

	if (capturefd_yuv420 > 0)
		if (-1 == close(capturefd_yuv420))
			errno_exit("close");

	if (capturefd_m > 0)
		if (-1 == close(capturefd_m))
			errno_exit("close");

	capturefd = -1;
	capturefd_m = -1;
	capturefd_yuv420 = -1;
}

void probe_commit()
{
	int32_t arg;
	struct v4l2_format fmt;

	DPRINT("probe_commit %d %d %d %d %d\n", g_vscommit.bformatindex,
	       g_vscommit.bframeindex, g_vscommit.dwframeinterval,
	       g_vscommit.dwmaxvideoframesize,
	       g_vscommit.dwmaxpayloadtransfersize);

	commitfs.width = g_vscommit.bformatindex;
	commitfs.height = g_vscommit.bframeindex;

	if (xioctl(android_fd, ANDROIDUVC_IOC_QUERYFORMAT, &commitfs) != 0)
		errno_exit("ANDROIDUVC_IOC_QUERYFORMAT");

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = commitfs.width;
	fmt.fmt.pix.height = commitfs.height;
	fmt.fmt.pix.pixelformat = commitfs.type;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	DPRINT("Format %d %d %c%c%c%c\n", fmt.fmt.pix.width, fmt.fmt.pix.height,
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[0],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[1],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[2],
	       ((int8_t *) &(fmt.fmt.pix.pixelformat))[3]);

	if (xioctl(usbfd, VIDIOC_S_FMT, &fmt))
		errno_exit("capture VIDIOC_S_FMT");

	DPRINT("set format to uvc gadget\n");

	init_stuff(commitfs.width * commitfs.height * 2);

	DPRINT("set uvc gadget stream on\n");
	arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (-1 == xioctl(usbfd, VIDIOC_STREAMON, &arg)) {
		switch (errno) {
		case EAGAIN:
			return;
		case EIO:
		default:
			errno_exit("send_respond");
		}
	}

#ifndef LOAD_FROM_IMAGE_FILE
	open_capture_device(commitfs.type);
	init_capture_device();
	init_capture_buffers();
	start_capturing();
#endif
}

void get_ready_for_preview()
{
	int32_t ret;
	videostart = 1;
	ret = pthread_create(&videothid, NULL, (void *)video_thread, NULL);
	if (ret != 0) {
		DPRINT("Create pthread error!\n");
		return;
	}
}

#ifdef LOAD_FROM_IMAGE_FILE

void video_thread()
{
	fd_set outfds;
	int32_t epnum;
	struct timeval tm;
	int32_t count = 0;

#ifndef PREVIEW_PROBE_COMMIT
	while (count < 3 && videostart) {
		if (xioctl(android_fd, ANDROIDUVC_IOC_WAIT_NAK, &epnum))
			errno_exit("ANDROIDUVC_IOC_WAIT_NAK");

		if (epnum == 1)
			count++;
		else if (videostart)
			usleep(5000);

	}
#endif
	if (videostart == 0) {
		DPRINT("just return\n");
		return;
	}
	probe_commit();

	fill_stuff();

	DPRINT("enter video loop...\n");

	while (videostart) {
		int32_t r;

		struct pollfd fds[1];
		memset(fds, 0, sizeof(fds));

		fds[0].fd = usbfd;
		fds[0].events = POLLOUT | POLLERR;

		usleep(1);
		r = poll((struct pollfd *)&fds, 1, 1);
		if (r < 0)
			errno_exit("poll");

		if (r == 0)
			continue;

		if (fds[0].revents & POLLOUT) {


			struct v4l2_buffer buf;
			uint32_t i;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = V4L2_MEMORY_USERPTR;

			if (-1 == xioctl(usbfd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
				case EAGAIN:
					return;
				case EIO:
				default:
					break;
				}
			}

			for (i = 0; i < BUFFER_COUNT; ++i)
				if (buf.m.userptr ==
				    (unsigned long)usbbuffers[i].start
				    && buf.length == usbbuffers[i].length)
					break;

			if (i >= BUFFER_COUNT) {
				DPRINT("frame buffer %d\n", i);
				break;
			}

			if (-1 == xioctl(usbfd, VIDIOC_QBUF, &buf))
				break;
		}
	}

	DPRINT("exit video thread\n");

	int32_t i;
	for (i = 0; i < BUFFER_COUNT; ++i)
		free(usbbuffers[i].start);
}

#else

#ifdef DUMP_H264_FILE
static FILE *pFile;
static int dumpcount;

int save_data_to_file(char *data, int length)
{

	int len;
	struct stat statbuff;

	if (dumpcount >= 10) {
		fclose(pFile);
		pFile = fopen("/bin/h264.raw", "wb");
		if (NULL == pFile) {
			DPRINT("open file h264.raw failed\n");
			return -1;
		}
		dumpcount = 0;
	}

	len = fwrite(data, 1, length, pFile);
	dumpcount++;

	return 0;
}

#endif

void synccustomizedvars(void)
{
	int ret;

	brightnessitem.last = get_video_ctrl(capturefd, V4L2_CID_BRIGHTNESS);
	contrastitem.last = get_video_ctrl(capturefd, V4L2_CID_CONTRAST);
	hueitem.last = get_video_ctrl(capturefd, V4L2_CID_HUE);
	saturationitem.last = get_video_ctrl(capturefd, V4L2_CID_SATURATION);
	gammaitem.last = get_video_ctrl(capturefd, V4L2_CID_GAMMA);
	whitebalancetempitem.last = get_video_ctrl(capturefd, V4L2_CID_WHITE_BALANCE_TEMPERATURE);
	whitebalancetempautoitem.last = get_video_ctrl(capturefd, V4L2_CID_AUTO_WHITE_BALANCE);
	backlightcompitem.last = get_video_ctrl(capturefd, V4L2_CID_BACKLIGHT_COMPENSATION);
	gainitem.last = get_video_ctrl(capturefd, V4L2_CID_GAIN);
	pwrlinefreqitem.last = get_video_ctrl(capturefd, V4L2_CID_POWER_LINE_FREQUENCY);
	exposuretimeabsolutitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_ABSOLUTE);
	exposuretimeautoitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO);
	lowlightcompitem.last = get_video_ctrl(capturefd, V4L2_CID_EXPOSURE_AUTO_PRIORITY);
	panitem.last = get_video_ctrl(capturefd, V4L2_CID_PAN_ABSOLUTE);
	tiltitem.last = get_video_ctrl(capturefd, V4L2_CID_TILT_ABSOLUTE);
	zoomitem.last = get_video_ctrl(capturefd, V4L2_CID_ZOOM_ABSOLUTE);
	rollitem.last = get_video_ctrl(capturefd, V4L2_CID_ROLL_ABSOLUTE);
	sharpnessitem.last = get_video_ctrl(capturefd, V4L2_CID_ROLL_ABSOLUTE);

#ifdef _AF_ENABLE_
	focusabsolutitem.last = get_video_ctrl(capturefd, V4L2_CID_FOCUS_ABSOLUTE);
	focusautoitem.last = get_video_ctrl(capturefd, V4L2_CID_FOCUS_AUTO);
#endif

}

void video_thread()
{
	fd_set outfds, infds;
	int32_t epnum;
	struct timeval tm;
	int32_t count = 0;
	int32_t fd;
	struct v4l2_buffer h264buf;
	uint8_t *h264bufstart = 0;
	uint32_t h264buflen;
	struct stat statbuff;

	DPRINT("enter video_thread %x %x\n", pthread_self(), getpid());

#ifdef DUMP_H264_FILE
	if (pFile) {
		stat("/bin/h264.raw", &statbuff);
		DPRINT("close %d\n", statbuff.st_size);

		fflush(pFile);
		fclose(pFile);
		stat("h264.raw", &statbuff);

		DPRINT("total %d\n", statbuff.st_size);
	}
#endif

#ifndef PREVIEW_PROBE_COMMIT
	while (count < 3 && videostart) {
		if (xioctl(android_fd, ANDROIDUVC_IOC_WAIT_NAK, &epnum))
			errno_exit("ANDROIDUVC_IOC_WAIT_NAK");

		if (epnum == 1)
			count++;
		else if (videostart)
			usleep(5000);

	}
#endif
	if (videostart == 0) {
		DPRINT("just return\n");
		return;
	}

	probe_commit();

#ifdef DUMP_H264_FILE
	pFile = fopen("/bin/h264.raw", "wb");
	if (NULL == pFile) {
		DPRINT("open file h264.raw failed\n");
		return -1;
	}
	DPRINT("open dump file\n");
#endif
	DPRINT("enter video loop...\n");

	synccustomizedvars();

	while (videostart) {
		int32_t r;

		struct pollfd fds[2];
		memset(fds, 0, sizeof(fds));

		fds[0].fd = usbfd;
		fds[0].events = POLLOUT | POLLERR;

		fds[1].fd = capturefd;
		fds[1].events = POLLIN | POLLERR;

		usleep(1);

		r = poll((struct pollfd *)&fds, 2, 1);
		if (r < 0)
			errno_exit("poll");

		if (r == 0)
			continue;

		if (fds[0].revents & POLLOUT) {
			struct v4l2_buffer buf;
			uint32_t i;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = V4L2_MEMORY_USERPTR;

			DPRINT("USB VIDIOC_DQBUF\n");

_USB_DQBUF_EAGAIN:

			if (-1 == xioctl(usbfd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
				case EAGAIN:
					usleep(1000);
					goto _USB_DQBUF_EAGAIN;
				default:
					DPRINT("error %d, %s\n", errno, strerror(errno));
					goto failed;
				}
			}
			DPRINT("USB VIDIOC_DQBUF finished:%d\n", buf.index);

			struct frame_list *captureframe;
			captureframe =
			    list_first_entry(&capturelist, struct frame_list,
					     list);
			if (captureframe == 0)
				errno_exit("captureframe list is empty");

_CAPTURE_QBUF_EAGAIN:

			if (-1 ==
			    xioctl(capturefd, VIDIOC_QBUF,
				   &captureframe->v4l2buf)) {
				switch (errno) {
				case EAGAIN:
					usleep(1000);
					goto _CAPTURE_QBUF_EAGAIN;
				default:
					DPRINT("error %d, %s\n", errno, strerror(errno));
					goto failed;
				}
			}

			DPRINT("Capture VIDIOC_QBUF finished\n");

			list_del(&captureframe->list);

			h264bufstart = 0;

			free(captureframe);
		}

		if (fds[1].revents & POLLIN) {
			struct frame_list *captureframe =
			    malloc(sizeof(struct frame_list));
			memset(captureframe, 0, sizeof(struct frame_list));

			captureframe->v4l2buf.type =
			    V4L2_BUF_TYPE_VIDEO_CAPTURE;
			captureframe->v4l2buf.memory = V4L2_MEMORY_MMAP;
			DPRINT("Capture VIDIOC_DQBUF\n");
_CAPTURE_DQBUF_EAGAIN:
			if (-1 ==
			    xioctl(capturefd, VIDIOC_DQBUF,
				   &captureframe->v4l2buf)) {
				switch (errno) {
				case EAGAIN:
					usleep(1000);
					goto _CAPTURE_DQBUF_EAGAIN;
				default:
					DPRINT("error %d, %s\n", errno, strerror(errno));
					goto failed;
				}
			}
			list_add_tail(&captureframe->list, &capturelist);

			if (commitfs.type != V4L2_PIX_FMT_H264) {

				struct v4l2_buffer v4l2buf;

				v4l2buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
				v4l2buf.memory = V4L2_MEMORY_USERPTR;
				v4l2buf.index = captureframe->v4l2buf.index;
				v4l2buf.m.userptr =
				    (unsigned long)capturebuffers[captureframe->
								  v4l2buf.index].
				    start;
				v4l2buf.length =
				    capturebuffers[captureframe->v4l2buf.index].
				    length;
				v4l2buf.bytesused =
				    captureframe->v4l2buf.bytesused;
				v4l2buf.reserved2 = 0;
				v4l2buf.reserved = captureframe->v4l2buf.reserved;

#ifdef DUMP_H264_FILE
				save_data_to_file(v4l2buf.m.userptr, v4l2buf.bytesused);
#endif


_USB_QBUF_EAGAIN:
				if (-1 == xioctl(usbfd, VIDIOC_QBUF, &v4l2buf)) {
					switch (errno) {
					case EAGAIN:
						usleep(1000);
						goto _USB_QBUF_EAGAIN;
					default:
						DPRINT("error %d, %s\n", errno, strerror(errno));
						goto failed;
					}
				}
			}

		}

		if (commitfs.type == V4L2_PIX_FMT_H264) {
			if (h264bufstart == 0) {

				if (list_empty(&capturelist))
					continue;

				struct frame_list *captureframe;
				captureframe =
				    list_first_entry(&capturelist,
						     struct frame_list, list);

				uvc_h264enc_frame(capturebuffers
						  [captureframe->v4l2buf.index].
						  start, &h264bufstart,
						  &h264buflen);


				CLEAR(h264buf);
#ifdef DUMP_H264_FILE
				save_data_to_file(h264bufstart, h264buflen);
#endif
				h264buf.index = captureframe->v4l2buf.index;

				if ((uint32_t) h264bufstart > getoutbufstart()) {
					h264bufstart -= 48;
					h264buf.reserved2 = 1;
					DPRINT("264 VIDIOC_QBUF p %d\n", h264buf.index);

				} else {
					h264buf.reserved2 = 0;
					DPRINT("264 VIDIOC_QBUF i %d\n", h264buf.index);

				}

				h264buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
				h264buf.memory = V4L2_MEMORY_USERPTR;
				h264buf.m.userptr = (unsigned long)h264bufstart;
				h264buf.length = getoutbufsize();
				h264buf.bytesused = h264buflen;
				h264buf.reserved =
				    captureframe->v4l2buf.reserved;

_H264_QBUF_EAGAIN:
				if (-1 == xioctl(usbfd, VIDIOC_QBUF, &h264buf)) {
					switch (errno) {
					case EAGAIN:
						usleep(5000);
						goto _H264_QBUF_EAGAIN;
					default:
						DPRINT("error %d, %s\n", errno, strerror(errno));
						goto failed;
					}
				}

				DPRINT("264 VIDIOC_QBUF %x %x %x\n", h264buf.m.userptr, h264buf.bytesused, h264buf.length);

			}

		}
	}

failed:

	DPRINT("exit video thread\n");
#ifdef DUMP_H264_FILE
	dumpcount = 0;
#endif
	while (!list_empty(&capturelist)) {
		struct frame_list *frame;
		frame = list_first_entry(&capturelist, struct frame_list, list);
		list_del(&frame->list);
		free(frame);
	}

	if (commitfs.type == V4L2_PIX_FMT_H264) {
		DPRINT("deinit h264 encoder\n");
		uvc_h264enc_close();
	}
	stop_capturing();
	uninit_capture_device();
	close_capture_device();
}

#endif

void usb_thread()
{
	int32_t arg;
	int32_t ret;

	DPRINT("enter uvc loop... %x %x\n", pthread_self(), getpid());

	for (; mainloop; ) {

		int32_t r;

		struct pollfd fds[1];
		memset(fds, 0, sizeof(fds));
		fds[0].fd = usbfd;
		fds[0].events = POLLPRI;

		r = poll((struct pollfd *)&fds, 1, 1);
		if (r < 0) {
			DPRINT("poll error\n");
			errno_exit("poll");
		}
		if (r == 0)
			continue;

		if (fds[0].revents & POLLPRI) {
			if (-1 == xioctl(usbfd, VIDIOC_DQEVENT, &event_now)) {
				switch (errno) {
				case EAGAIN:
					return;
				case EIO:
				default:
					errno_exit("VIDIOC_DQBUF");
				}
			}
			arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;

			if (event_now.type == UVC_EVENT_STREAMON) {
				xioctl(usbfd, VIDIOC_STREAMON, &arg);
			} else if (event_now.type == UVC_EVENT_STREAMOFF) {
				DPRINT("stream off\n");
				if (videostart == 1) {
					DPRINT("re create video thread\n");

					videostart = 0;
					pthread_join(videothid, NULL);

					xioctl(usbfd, VIDIOC_STREAMOFF, &arg);
#ifndef PREVIEW_PROBE_COMMIT
					get_ready_for_preview();
#endif
				}
			} else if (event_now.type == UVC_EVENT_SETUP) {
				struct usb_ctrlrequest *req;

				memcpy(&event_setup,
				       &event_now, sizeof(event_now));
				req = (struct usb_ctrlrequest *)
				    event_setup.u.data;

				g_brequesttype = req->brequesttype;
				g_brequest = req->brequest;
				g_wvalue = req->wvalue;
				g_windex = req->windex;
				g_wlength = req->wlength;
				g_wvalue_h = ((int8_t *) &(req->wvalue))[1];
				g_wvalue_l = ((int8_t *) &(req->wvalue))[0];

				g_windex_h = ((int8_t *) &(req->windex))[1];
				g_windex_l = ((int8_t *) &(req->windex))[0];
/*
				DPRINT("t:%x %x w:%x i:%x l:%x\n",
				       g_brequesttype, g_brequest, g_wvalue,
				       g_windex, g_wlength);
*/
				g_setup_phrase = 1;
/*
				if (g_brequest == SET_CUR) {
					send_out_respond();
					continue;
				}
*/
				setup_process();

			} else if (event_now.type == UVC_EVENT_DATA) {

				DPRINT("UVC_EVENT_DATA\n");

				g_setup_phrase = 0;

				ep0outdataindex = 6;
				memcpy(&event_data,
				       &event_now, sizeof(event_now));
				setup_process();
			} else if (event_now.type == UVC_EVENT_DISCONNECT) {
				DPRINT("stream disconnect\n");

				if (videostart == 1) {
					DPRINT("close stream\n");
					videostart = 0;
					pthread_join(videothid, NULL);
					xioctl(usbfd, VIDIOC_STREAMOFF, &arg);
#ifndef PREVIEW_PROBE_COMMIT
					get_ready_for_preview();
#endif

				}

			} else
				errno_exit("error type");
		}
	}
}

static int32_t list_fps(int32_t fd, int32_t fmt, int32_t w, int32_t h)
{
	int32_t ret = 0;
	struct v4l2_frmivalenum v4l2_frmival;
	struct format_setting fs;

	memset(&fs, 0, sizeof(fs));
	memset(&v4l2_frmival, 0, sizeof(v4l2_frmival));
	v4l2_frmival.pixel_format = fmt;
	v4l2_frmival.width = w;
	v4l2_frmival.height = h;
	v4l2_frmival.index = 0;

	DPRINT("listfps %d %d %c%c%c%c\n", w, h,
	       ((int8_t *) &(commitfs.type))[0],
	       ((int8_t *) &(commitfs.type))[1],
	       ((int8_t *) &(commitfs.type))[2],
	       ((int8_t *) &(commitfs.type))[3]);
	do {
		ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &v4l2_frmival);
		if (ret < 0)
			break;
		if (V4L2_FRMIVAL_TYPE_DISCRETE == v4l2_frmival.type) {
			DPRINT("disc fps %d %d/%d\n", v4l2_frmival.index,
			       v4l2_frmival.discrete.numerator,
			       v4l2_frmival.discrete.denominator);
			v4l2_frmival.index++;

			int32_t i;
			for (i = 0;
			     i < (sizeof(fpsarray) / sizeof((fpsarray)[0]));
			     i++)
				if (v4l2_frmival.discrete.denominator ==
				    fpsarray[i])
					fs.fps |= 1 << i;

		} else {
			DPRINT("[FPS:]min:%d/%d, max:%d/%d, step:%d/%d\n",
				v4l2_frmival.stepwise.min.numerator,
				v4l2_frmival.stepwise.min.denominator,
				v4l2_frmival.stepwise.max.numerator,
				v4l2_frmival.stepwise.max.denominator,
				v4l2_frmival.stepwise.step.numerator,
				v4l2_frmival.stepwise.step.denominator);
			break;
		}
	} while (1);

	if (fs.fps == 0) {
		DPRINT("sensor not support\n");
		return -1;
	}

	fs.height = h;
	fs.width = w;
	fs.type = fmt;

	ret = xioctl(android_fd, ANDROIDUVC_IOC_SETFORMAT, &fs);
	if (ret) {
		DPRINT("set format failed, %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return 0;
}

static int32_t list_resolutions(int32_t fd, int32_t fmt)
{
	int32_t ret = 0;
	struct v4l2_frmsizeenum v4l2_frmsize;
	int32_t w, h;

	memset(&v4l2_frmsize, 0, sizeof(v4l2_frmsize));
	v4l2_frmsize.index = 0;
	v4l2_frmsize.pixel_format = fmt;
	do {
		ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &v4l2_frmsize);
		if (ret < 0)
			break;
		if (V4L2_FRMSIZE_TYPE_DISCRETE == v4l2_frmsize.type) {
			DPRINT("width = %4d, height = %4d\n",
			       v4l2_frmsize.discrete.width,
			       v4l2_frmsize.discrete.height);
			list_fps(fd, fmt, v4l2_frmsize.discrete.width,
				 v4l2_frmsize.discrete.height);
			v4l2_frmsize.index++;
		} else {
			DPRINT
			    ("[Resolution:]min_w = %d, max_w = %d, step_w = %d, min_h = %d, max_h = %d, step_h = %d\n",
			     v4l2_frmsize.stepwise.min_width,
			     v4l2_frmsize.stepwise.max_width,
			     v4l2_frmsize.stepwise.step_width,
			     v4l2_frmsize.stepwise.min_height,
			     v4l2_frmsize.stepwise.max_height,
			     v4l2_frmsize.stepwise.step_height);

			for (w = v4l2_frmsize.stepwise.min_width;
			     w <= v4l2_frmsize.stepwise.max_width;
			     w += v4l2_frmsize.stepwise.step_width) {
				for (h = v4l2_frmsize.stepwise.min_height;
				     h <= v4l2_frmsize.stepwise.max_height;
				     h += v4l2_frmsize.stepwise.step_height) {
					list_fps(fd, fmt, w, h);
				}
			}

			break;
		}
	} while (1);
	return 0;
}

int list_stream_info(int fmt, int w, int h, int fps)
{
#ifdef _GET_FMTDESC_FROM_V4L2_

	struct v4l2_fmtdesc fmtdesc;
	int ret;
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	open_capture_device(fmt);

	do {
		ret = xioctl(capturefd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (ret < 0)
			break;

		DPRINT("format-[%d]-%s----\n", fmtdesc.index,
		       fmtdesc.description);
		list_resolutions(fd, fmtdesc.pixelformat);
		fmtdesc.index++;
	} while (1);
	return 0;
#else
	struct format_setting fs;
	int ret;
	int i;

	fs.height = h;
	fs.width = w;
	fs.type = fmt;
	fs.fps = fps;
	DPRINT("set format: %c%c%c%c-%d-%d-%x\n",
	       ((int8_t *) &(fmt))[0], ((int8_t *) &(fmt))[1],
	       ((int8_t *) &(fmt))[2], ((int8_t *) &(fmt))[3], w, h, fps);

	ret = xioctl(android_fd, ANDROIDUVC_IOC_SETFORMAT, &fs);
	if (ret) {
		DPRINT("set format failed, %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif
}

int32_t openuvcfile()
{
	struct stat st;
	int8_t *android_uvc = "/dev/android_uvc";

	if (-1 == stat(android_uvc, &st)) {
		DPRINT("cannot identify '%s': %d, %s\n",
		       android_uvc, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	android_fd = open(android_uvc, O_RDWR | O_NONBLOCK, 0);
	if (-1 == android_fd) {
		DPRINT("cannot open '%s': %d, %s\n",
		       android_uvc, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return 0;
}

void closeuvcfile()
{
	close(android_fd);
}

void setuvcformat()
{
	int32_t ret = 0;
	int32_t i;

	openuvcfile();

	ret = xioctl(android_fd, ANDROIDUVC_IOC_CLRFORMAT, &ret);
	if (ret) {
		DPRINT("clear format failed, %d, %s, %d\n",
		       errno, strerror(errno), ret);
		exit(EXIT_FAILURE);
	}

#ifdef _GET_FMTDESC_FROM_V4L2_
	list_stream_info(V4L2_PIX_FMT_NV12, 0, 0, 0, 0);
	list_stream_info(V4L2_PIX_FMT_MJPEG, 0, 0, 0, 0);
#else
#ifdef LOAD_FROM_IMAGE_FILE
	list_stream_info(g_type, g_width, g_height, g_fps);
#else
	list_stream_info(V4L2_PIX_FMT_NV12, 640, 480,	FPS_30 | FPS_15);
	list_stream_info(V4L2_PIX_FMT_NV12, 1280, 720, FPS_30 | FPS_15);
	list_stream_info(V4L2_PIX_FMT_H264, 640, 480,	FPS_30 | FPS_15);
	list_stream_info(V4L2_PIX_FMT_H264, 1280, 720, FPS_30 | FPS_15);
	list_stream_info(V4L2_PIX_FMT_MJPEG, 640, 480, FPS_30 | FPS_15);
	list_stream_info(V4L2_PIX_FMT_MJPEG, 1280, 720, FPS_30 | FPS_15);
#endif
#endif
	ret = xioctl(android_fd, ANDROIDUVC_IOC_PRODUCE_DESC, NULL);
	if (ret) {
		DPRINT("set format failed, %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
#ifndef LOAD_FROM_IMAGE_FILE
	close_capture_device();
#endif
	closeuvcfile();

}

static void program_quit(int sign)
{
	DPRINT("testuvc terminated\n");
	videostart = 0;
	mainloop = 0;
}

void initcamera()
{
	open_capture_device(V4L2_PIX_FMT_NV12);

	if (rts_read_xmem(capturefd, 0xfe9c, 1, &g_byhwid1)) {
		DPRINT("read version id1 error\n");
		return;
	}

	if (rts_read_xmem(capturefd, 0xfe9d, 1, &g_byhwid1)) {
		DPRINT("read version id2 error\n");
		return;
	}

	initcustomizedvars();

	close_capture_device();
}

int32_t main(int32_t argc, int8_t **argv)
{
	int32_t ret;

	int opt;
	char *optstring = "m:y:";

	strcpy(capture_dev_mjpeg, "/dev/video");
	strcpy(capture_dev_yuv420, "/dev/video");

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
		case 'm':
			strcat(capture_dev_mjpeg, optarg);
			break;
		case 'y':
			strcat(capture_dev_yuv420, optarg);
			break;
		default:
			DPRINT("Usage:\n");
			DPRINT("Start isp video stream forward:\n");
			DPRINT("	testuvc -m 50 -y 51\n");
			break;
		}
	}

	strcpy(usb_dev, "/dev/video41");

#ifdef LOAD_FROM_IMAGE_FILE
	g_type = V4L2_PIX_FMT_YUYV;
	g_width = 640;
	g_height = 480;
	g_fps = FPS_30;
#endif
	setuvcformat();

	signal(SIGINT, program_quit);
	signal(SIGTERM, program_quit);
	signal(SIGQUIT, program_quit);
	signal(SIGTSTP, program_quit);
	signal(SIGPIPE, SIG_IGN);

	usleep(10000);

	initcamera();
	openuvcfile();
	open_usb_device();

	ret = pthread_create(&eventthid, NULL, (void *)usb_thread, NULL);
	if (ret != 0) {
		DPRINT("Create pthread error!\n");
		return -1;
	}

	DPRINT("Create usb pthread:%x\n", eventthid);

#ifndef PREVIEW_PROBE_COMMIT
	get_ready_for_preview();
#endif

	pthread_join(eventthid, NULL);

	DPRINT("event thread exit\n");

	close_usb_device();
	closeuvcfile();

	return 0;
}
