/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_video.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/pm_runtime.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>
#include "rts-dma-contig.h"
#include "rts_camera_priv.h"

int rtscam_convert_addr_from_vm_to_phy(struct rtscam_video_device *icd,
		unsigned long vm_addr,
		unsigned long *phy_addr)
{
	struct vm_area_struct *vma = NULL;
	void *buf = NULL;
	int found = 0;
	int i;

	if (!icd->alloc_ctx || (icd->mem_ops != &vb2_dma_contig_memops &&
		icd->mem_ops != &rts_dma_contig_memops))
		return -EINVAL;

	if (!vm_addr)
		return -EINVAL;

	vma = find_vma(current->mm, vm_addr);
	if (NULL == vma)
		return -EINVAL;

	for (i = 0; i < icd->streamnum; i++) {
		struct rtscam_video_stream *stream = icd->streams + i;
		if (vma->vm_file == stream->memory_owner) {
			found = 1;
			break;
		}
	}
	if (!found) {
		rtsprintk(RTS_TRACE_ERROR, "not rts camera video buffer\n");
		return -EINVAL;
	}

	buf = icd->mem_ops->get_userptr(icd->alloc_ctx,
			vm_addr, vma->vm_end - vma->vm_start,
			!V4L2_TYPE_IS_OUTPUT(icd->type));

	if (!buf)
		return -EINVAL;

	if (phy_addr)
		*phy_addr = *(dma_addr_t *) icd->mem_ops->cookie(buf);

	icd->mem_ops->put_userptr(buf);

	return 0;
}

int rtscam_get_phy_addr(struct vb2_buffer *vb, dma_addr_t *phy_addr)
{
	struct rtscam_video_stream *stream = vb2_get_drv_priv(vb->vb2_queue);
	struct rtscam_video_device *icd = stream->icd;
	dma_addr_t *dma_addr = NULL;

	if (!icd->alloc_ctx || (icd->mem_ops != &vb2_dma_contig_memops &&
			icd->mem_ops != &rts_dma_contig_memops))
		return -EINVAL;

	dma_addr = vb2_plane_cookie(vb, 0);
	if (!dma_addr)
		return -EINVAL;

	if (phy_addr)
		*phy_addr = *dma_addr;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_get_phy_addr);

void rtscam_init_capture_buffer(struct rtscam_video_stream *stream)
{
	spin_lock_irq(&stream->lock);
	INIT_LIST_HEAD(&stream->capture);
	rtscam_call_video_op(stream->icd, init_capture_buffers, stream);
	spin_unlock_irq(&stream->lock);
}

int rtscam_submit_buffer(struct rtscam_video_stream *stream,
		struct rtscam_video_buffer *buf)
{
	struct rtscam_video_buffer *rbuf = NULL;
	int ret;

	if (buf)
		list_add_tail(&buf->list, &stream->capture);

	if (list_empty(&stream->capture))
			return -EINVAL;

	rbuf = list_first_entry(&stream->capture,
			struct rtscam_video_buffer, list);

	ret = rtscam_call_video_op(stream->icd, submit_buffer, stream, rbuf);
	if (ret)
		return ret;
	list_del_init(&rbuf->list);

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_submit_buffer);

void rtscam_buffer_done(struct rtscam_video_stream *stream,
		struct rtscam_video_buffer *rbuf, unsigned long bytesused)
{
	struct rtscam_video_format *format;

	if (!stream || !rbuf)
		return;

	format = find_format_by_fourcc(stream, stream->user_format);
	if (format)
		rbuf->vb.v4l2_buf.field = format->field;
	else
		rbuf->vb.v4l2_buf.field = V4L2_FIELD_NONE;
	rbuf->vb.v4l2_buf.sequence = stream->sequence++;

	rtscam_get_timestamp(&rbuf->vb.v4l2_buf.timestamp);
	rtsprintk(RTS_TRACE_PTS, "0x%08x\n", rbuf->pts);
	vb2_set_plane_payload(&rbuf->vb, 0, bytesused);
	vb2_buffer_done(&rbuf->vb, VB2_BUF_STATE_DONE);
}
EXPORT_SYMBOL_GPL(rtscam_buffer_done);

int rtscam_queue_setup(struct vb2_queue *q, const struct v4l2_format *fmt,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], void *alloc_ctxs[])
{
	struct rtscam_video_stream *stream = vb2_get_drv_priv(q);

	if (fmt) {
		const struct v4l2_pix_format *pix = &fmt->fmt.pix;
		struct rtscam_video_format *format = NULL;

		format = find_format_by_fourcc(stream,
				fmt->fmt.pix.pixelformat);
		if (!format)
			return -EINVAL;

		sizes[0] = ((pix->width * format->bpp) >> 3) * pix->height;
	} else {
		sizes[0] = stream->sizeimage;
	}

	if (sizes[0] == 0) {
		rtsprintk(RTS_TRACE_ERROR, "frame buf size is 0\n");
		return -EINVAL;
	}

	alloc_ctxs[0] = stream->icd->alloc_ctx;

	if (!(*num_buffers))
		*num_buffers = 2;

	if (*num_buffers > RTSCAM_MAX_BUFFER_NUM)
		*num_buffers = RTSCAM_MAX_BUFFER_NUM;

	*num_planes = 1;

	return 0;
}

int rtscam_buf_init(struct vb2_buffer *vb)
{
	return 0;
}

int rtscam_buf_prepare(struct vb2_buffer *vb)
{
	return 0;
}

int rtscam_buf_finish(struct vb2_buffer *vb)
{
	return 0;
}

void rtscam_buf_cleanup(struct vb2_buffer *vb)
{
}

int rtscam_buf_start_streaming(struct vb2_queue *q, unsigned int count)
{
	return 0;
}

int rtscam_buf_stop_stream(struct vb2_queue *q)
{
	return 0;
}

void rtscam_buf_wait_prepare(struct vb2_queue *q)
{
	/*unlock*/
	struct rtscam_video_stream *stream = vb2_get_drv_priv(q);
	mutex_unlock(&stream->queue_lock);
}

void rtscam_buf_wait_finish(struct vb2_queue *q)
{
	/*lock*/
	struct rtscam_video_stream *stream = vb2_get_drv_priv(q);
	mutex_lock(&stream->queue_lock);
}

void rtscam_buf_queue(struct vb2_buffer *vb)
{
	struct rtscam_video_stream *stream = vb2_get_drv_priv(vb->vb2_queue);
	struct rtscam_video_buffer *buf = to_rtscam_vbuf(vb);

	spin_lock_irq(&stream->lock);
	rtscam_submit_buffer(stream, buf);
	spin_unlock_irq(&stream->lock);
}

static struct vb2_ops rtscam_vb2_ops = {
	.queue_setup = rtscam_queue_setup,
	.wait_prepare = rtscam_buf_wait_prepare,
	.wait_finish = rtscam_buf_wait_finish,
	.buf_init = rtscam_buf_init,
	.buf_prepare = rtscam_buf_prepare,
	.buf_finish = rtscam_buf_finish,
	.buf_cleanup = rtscam_buf_cleanup,
	.start_streaming = rtscam_buf_start_streaming,
	.stop_streaming = rtscam_buf_stop_stream,
	.buf_queue = rtscam_buf_queue,
};

static int rtscam_video_init_videobuf2(struct vb2_queue *queue,
		struct rtscam_video_stream *stream)
{
	int ret;
	struct rtscam_video_device *icd = stream->icd;

	queue->type = icd->type;
	queue->io_modes = VB2_MMAP | VB2_USERPTR;
	queue->drv_priv = stream;
	queue->ops = &rtscam_vb2_ops;
	queue->mem_ops = icd->mem_ops;
	queue->buf_struct_size = sizeof(struct rtscam_video_buffer);
	queue->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(queue);
	if (ret)
		return ret;

	mutex_init(&stream->queue_lock);
	INIT_LIST_HEAD(&stream->capture);
	spin_lock_init(&stream->lock);

	return 0;
}

static void rtscam_video_release_videobuf2(struct vb2_queue *queue,
		struct rtscam_video_stream *stream)
{
}

static int rtscam_video_alloc_buffers(struct rtscam_video_stream *stream,
		struct file *file, struct v4l2_requestbuffers *p)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_reqbufs(&stream->vb2_vidp, p);
	if (!ret)
		rtscam_init_capture_buffer(stream);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static void rtscam_video_free_buffers(struct rtscam_video_stream *stream)
{
	mutex_lock(&stream->queue_lock);
	vb2_queue_release(&stream->vb2_vidp);
	mutex_unlock(&stream->queue_lock);
}

static int rtscam_video_query_buffer(struct rtscam_video_stream *stream,
		struct v4l2_buffer *buf)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_querybuf(&stream->vb2_vidp, buf);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_queue_buffer(struct rtscam_video_stream *stream,
		struct v4l2_buffer *buf)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_qbuf(&stream->vb2_vidp, buf);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_dequeue_buffer(struct rtscam_video_stream *stream,
		struct v4l2_buffer *buf, int nonblocking)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_dqbuf(&stream->vb2_vidp, buf, nonblocking);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_queue_mmap(struct rtscam_video_stream *stream,
		struct vm_area_struct *vma)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_mmap(&stream->vb2_vidp, vma);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_queue_poll(struct rtscam_video_stream *stream,
		struct file *file, poll_table *wait)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	ret = vb2_poll(&stream->vb2_vidp, file, wait);
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_queue_allocated(struct rtscam_video_stream *stream)
{
	int allocated;

	mutex_lock(&stream->queue_lock);
	allocated = vb2_is_busy(&stream->vb2_vidp);
	mutex_unlock(&stream->queue_lock);

	return allocated;
}

static int rtscam_video_queue_enable(struct rtscam_video_stream *stream, int enable)
{
	int ret;

	mutex_lock(&stream->queue_lock);
	if (enable) {
		ret = vb2_streamon(&stream->vb2_vidp, stream->vb2_vidp.type);
		if (ret < 0)
			goto done;
	} else {
		ret = vb2_streamoff(&stream->vb2_vidp, stream->vb2_vidp.type);
		if (ret < 0)
			goto done;
	}

done:
	mutex_unlock(&stream->queue_lock);

	return ret;
}

static int rtscam_video_querycap(struct rtscam_video_stream *stream,
		struct v4l2_capability *cap)
{
	strlcpy(cap->driver, stream->icd->drv_name, sizeof(cap->driver));
	if (stream->icd->dev_name)
		strlcpy(cap->card, stream->icd->dev_name, sizeof(cap->card));
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

static int rtscam_video_enum_fmt(struct rtscam_video_stream *stream,
		struct v4l2_fmtdesc *f)
{
	struct rtscam_video_format *format;
	int index = 0;

	if (f->type != stream->icd->type)
		return -EINVAL;

	if (f->index < 0)
		f->index = 0;

	format = stream->user_formats;
	index = 0;

	while (format) {
		if (index == f->index)
			break;
		format = format->next;
		index++;
	}

	if (!format)
		return -EINVAL;

	strlcpy(f->description, format->name, sizeof(f->description));
	f->pixelformat = format->fourcc;

	return 0;
}

static int rtscam_video_try_fmt(struct rtscam_video_stream *stream,
		struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int ret;

	if (f->type != stream->icd->type)
		return -EINVAL;

	ret = rtscam_try_user_format(stream, pix->pixelformat,
			pix->width, pix->height);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
				"format (%c%c%c%c) (%dx%d) unsupportted\n",
				v4l2pixfmtstr(pix->pixelformat),
				pix->width, pix->height);
		return ret;
	}

	return 0;
}

static int rtscam_video_set_fmt(struct rtscam_video_stream *stream,
		struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct rtscam_video_format *fmt;
	int ret;

	ret = rtscam_video_try_fmt(stream, f);
	if (ret)
		return ret;

	mutex_lock(&stream->stream_lock);

	if (rtscam_video_queue_allocated(stream)) {
		ret = -EBUSY;
		goto done;
	}

	if (vb2_is_streaming(&stream->vb2_vidp)) {
		ret = -EBUSY;
		goto done;
	}

	fmt = find_format_by_fourcc(stream, pix->pixelformat);

	ret = rtscam_set_user_format(stream, pix->pixelformat,
			pix->width, pix->height);
	if (ret)
		goto done;

	pix->field = fmt->field;
	pix->bytesperline = stream->bytesperline;
	pix->sizeimage = stream->sizeimage;

done:
	mutex_unlock(&stream->stream_lock);
	return ret;
}

static int rtscam_video_get_fmt(struct rtscam_video_stream *stream,
		struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct rtscam_video_format *fmt;

	if (f->type != stream->icd->type)
		return -EINVAL;

	fmt = find_format_by_fourcc(stream, stream->user_format);
	if (!fmt)
		return -EINVAL;

	pix->pixelformat = stream->user_format;
	pix->width = stream->user_width;
	pix->height = stream->user_height;
	pix->bytesperline = stream->bytesperline;
	pix->sizeimage = stream->sizeimage;
	pix->field = fmt->field;
	pix->colorspace = fmt->colorspace;

	return 0;
}

static int rtscam_video_enum_framesizes(struct rtscam_video_stream *stream,
		struct v4l2_frmsizeenum *fsize)
{
	struct rtscam_video_format *format = NULL;

	format = find_format_by_fourcc(stream, fsize->pixel_format);
	if (!format || !format->initialized)
		return -EINVAL;

	if (RTSCAM_SIZE_DISCRETE == format->frame_type) {
		struct rtscam_video_frame *frame = format->discrete.frames;
		int index = 0;

		while (frame) {
			if (fsize->index == index)
				break;
			frame = frame->next;
			index++;
		}
		if (!frame)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frame->size.width;
		fsize->discrete.height = frame->size.height;
	} else {
		if (fsize->index)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise.max_width = format->stepwise.max.width;
		fsize->stepwise.max_height = format->stepwise.max.height;
		fsize->stepwise.min_width = format->stepwise.min.width;
		fsize->stepwise.min_height = format->stepwise.min.height;
		fsize->stepwise.step_width = format->stepwise.step.width;
		fsize->stepwise.step_height = format->stepwise.step.height;
	}

	return 0;
}

static int rtscam_video_enum_frameintervals(struct rtscam_video_stream *stream,
		struct v4l2_frmivalenum *fival)
{
	struct rtscam_video_frmival *frmival = NULL;

	frmival = get_video_frmival(stream, fival->pixel_format,
			fival->width, fival->height);
	if (!frmival || !frmival->initialized)
		return -EINVAL;

	if (RTSCAM_SIZE_DISCRETE == frmival->frmival_type) {
		struct rtscam_frame_frmival *ival = frmival->discrete.frmivals;
		int index = 0;

		while (ival) {
			if (fival->index == index)
				break;
			ival = ival->next;
			index++;
		}
		if (!ival)
			return -EINVAL;

		fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
		fival->discrete.numerator = ival->frmival.numerator;
		fival->discrete.denominator = ival->frmival.denominator;
	} else {
		if (fival->index)
			return -EINVAL;

		fival->type = V4L2_FRMIVAL_TYPE_STEPWISE;
		fival->stepwise.max = frmival->stepwise.max;
		fival->stepwise.min = frmival->stepwise.min;
		fival->stepwise.step = frmival->stepwise.step;
	}

	return 0;
}

static int rtscam_video_get_parm(struct rtscam_video_stream *stream,
		struct v4l2_streamparm *parm)
{
	if (parm->type != stream->icd->type)
		return -EINVAL;

	parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	parm->parm.capture.capturemode = 0;
	parm->parm.capture.timeperframe.numerator = stream->user_numerator;
	parm->parm.capture.timeperframe.denominator = stream->user_denominator;
	parm->parm.capture.extendedmode = 0;
	parm->parm.capture.readbuffers = 0;

	return 0;
}

static int rtscam_video_enum_input(struct rtscam_video_stream *stream,
		struct v4l2_input *input)
{
	if (input->index)
		return -EINVAL;

	input->type = V4L2_INPUT_TYPE_CAMERA;
	strcpy(input->name, "Camera");

	return 0;
}

static int rtscam_video_get_input(struct rtscam_video_stream *stream,
		unsigned int *i)
{
	*i = 0;

	return 0;
}

static int rtscam_video_set_input(struct rtscam_video_stream *stream,
		unsigned int i)
{
	if (i)
		return -EINVAL;

	return 0;
}

static int rtscam_video_set_parm(struct rtscam_video_stream *stream,
		struct v4l2_streamparm *parm)
{
	int ret = 0;

	if (parm->type != stream->icd->type)
		return -EINVAL;

	ret = rtscam_set_user_frmival(stream,
			parm->parm.capture.timeperframe.numerator,
			parm->parm.capture.timeperframe.denominator);
	if (ret)
		return ret;

	return 0;
}

static int rtscam_video_enable(struct rtscam_video_stream *stream, int enable)
{
	int ret = 0;

	if (enable) {
		if (vb2_is_streaming(&stream->vb2_vidp))
			return -EINVAL;
		if (!rtscam_video_queue_allocated(stream))
			return -EINVAL;

		ret = rtscam_video_queue_enable(stream, 1);
		if (ret)
			return ret;
		ret = rtscam_call_video_op(stream->icd, s_stream, stream, 1);
		if (ret)
			rtscam_video_queue_enable(stream, 0);
		else
			stream->icd->streaming_count++;
	} else {
		if (!vb2_is_streaming(&stream->vb2_vidp))
			return 0;

		ret = rtscam_call_video_op(stream->icd, s_stream, stream, 0);
		if (!ret)
			ret = rtscam_video_queue_enable(stream, 0);

		if (!ret)
			stream->icd->streaming_count--;
	}

	return ret;
}

static int rtscam_acquire_privileges(struct rtscam_fh *handle)
{
	if (RTSCAM_STATE_ACTIVE == handle->state)
		return 0;

	if (atomic_inc_return(&handle->stream->active) != 1) {
		atomic_dec(&handle->stream->active);
		return -EBUSY;
	}

	handle->state = RTSCAM_STATE_ACTIVE;
	return 0;
}

static void rtscam_dismiss_privileges(struct rtscam_fh *handle)
{
	if (handle->state == RTSCAM_STATE_ACTIVE)
		atomic_dec(&handle->stream->active);

	handle->state = RTSCAM_STATE_PASSIVE;
}

static int rtscam_has_privileges(struct rtscam_fh *handle)
{
	return handle->state == RTSCAM_STATE_ACTIVE;
}

static int rtscam_video_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct rtscam_video_stream *stream;
	struct rtscam_video_device *icd;
	struct rtscam_fh *handle = NULL;
	int ret;

	if (!vdev || !video_is_registered(vdev))
		return -ENODEV;

	stream = video_get_drvdata(vdev);
	icd = stream->icd;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (NULL == handle)
		return -ENOMEM;

	ret = try_module_get(icd->ops->owner) ? 0 : -ENODEV;

	if (ret < 0) {
		rtsprintk(RTS_TRACE_ERROR,
				"couldn't lock capture driver\n");
		return ret;
	}

	if (mutex_lock_interruptible(&icd->dev_lock)) {
		ret = -ERESTARTSYS;
		goto elockdev;
	}

	if (atomic_inc_return(&icd->use_count) == 1) {
		ret = rtscam_call_video_op(icd, start_clock, icd);
		if (ret < 0) {
			rtsprintk(RTS_TRACE_ERROR,
					"couldn't activate the camera:%d\n",
					ret);
			atomic_dec(&icd->use_count);
			goto estartclock;
		}
	}

	if (atomic_inc_return(&stream->use_count) == 1) {
		pm_runtime_enable(&stream->vdev->dev);
		ret = pm_runtime_resume(&stream->vdev->dev);
		if (ret < 0 && ret != -ENOSYS) {
			rtsprintk(RTS_TRACE_ERROR, "pm resume failed\n");
			goto eresume;
		}
	}
	mutex_unlock(&icd->dev_lock);

	v4l2_fh_init(&handle->vfh, stream->vdev);
	v4l2_fh_add(&handle->vfh);
	handle->stream = stream;
	handle->state = RTSCAM_STATE_PASSIVE;
	file->private_data = handle;

	return 0;

eresume:
	if (atomic_dec_return(&stream->use_count) == 0)
		pm_runtime_disable(&stream->vdev->dev);
	if (atomic_dec_return(&icd->use_count) == 0)
		rtscam_call_video_op(icd, stop_clock, icd);
estartclock:
	mutex_unlock(&icd->dev_lock);
elockdev:
	module_put(icd->ops->owner);
	return ret;
}

static int rtscam_video_close(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct rtscam_video_stream *stream;
	struct rtscam_video_device *icd;
	struct rtscam_fh *handle = file->private_data;

	stream = video_get_drvdata(vdev);
	icd = stream->icd;

	if (rtscam_has_privileges(handle)) {
		/*stop streaming*/
		mutex_lock(&icd->dev_lock);
		mutex_lock(&stream->stream_lock);
		rtscam_video_enable(stream, 0);
		mutex_unlock(&stream->stream_lock);
		mutex_unlock(&icd->dev_lock);

		/*release buffer*/
		rtscam_video_free_buffers(stream);
	}

	rtscam_dismiss_privileges(handle);
	v4l2_fh_del(&handle->vfh);
	v4l2_fh_exit(&handle->vfh);
	kfree(handle);
	file->private_data = NULL;

	mutex_lock(&icd->dev_lock);
	if (atomic_dec_return(&stream->use_count) == 0) {
		pm_runtime_suspend(&stream->vdev->dev);
		pm_runtime_disable(&stream->vdev->dev);
	}

	if (atomic_dec_return(&icd->use_count) == 0)
		rtscam_call_video_op(icd, stop_clock, icd);
	mutex_unlock(&icd->dev_lock);
	module_put(icd->ops->owner);

	return 0;
}

long rtscam_video_do_ctrl_ioctl(struct rtscam_video_device *icd,
		unsigned int cmd, void *arg)
{
	long ret = 0;

	if (!icd)
		return -EINVAL;

	switch (cmd) {
	case VIDIOC_QUERYCTRL:
		ret = rtscam_query_v4l2_ctrl(icd, arg);
		break;
	case VIDIOC_G_CTRL:
		ret = rtscam_get_ctrl(icd, arg);
		break;
	case VIDIOC_S_CTRL:
		ret = rtscam_set_ctrl(icd, arg);
		break;
	case VIDIOC_G_EXT_CTRLS:
		ret = rtscam_get_ext_ctrls(icd, arg);
		break;
	case VIDIOC_S_EXT_CTRLS:
		ret = rtscam_set_ext_ctrls(icd, arg);
		break;
	case VIDIOC_TRY_EXT_CTRLS:
		ret = rtscam_try_ext_ctrls(icd, arg);
		break;
	case RTSCAMIOC_VENDOR_CMD:
		mutex_lock(&icd->dev_lock);
		ret = rtscam_call_video_op(icd, exec_command,
				icd->streams, arg);
		mutex_unlock(&icd->dev_lock);
		break;
	case RTSCAMIOC_GET_PHYADDDR:
	{
		unsigned long vm_addr = *(unsigned long *)arg;
		unsigned long phy_addr = 0;

		ret = rtscam_convert_addr_from_vm_to_phy(icd,
				vm_addr, &phy_addr);

		if (!ret)
			*(unsigned long *)arg = phy_addr;
		break;
	}
	default:
		rtsprintk(RTS_TRACE_ERROR,
			"Unknown[ctrl] ioctl 0x%08x, type = '%c' nr = 0x%x\n",
			cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		ret = -ENOTTY;
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(rtscam_video_do_ctrl_ioctl);

static long rtscam_video_do_ioctl(struct file *file,
		unsigned int cmd, void *arg)
{
	struct rtscam_video_stream *stream = video_drvdata(file);
	struct rtscam_video_device *icd = stream->icd;
	struct rtscam_fh *handle = file->private_data;
	long ret = 0;

	switch (cmd) {
	case VIDIOC_QUERYCAP:
		ret = rtscam_video_querycap(stream, arg);
		break;
	case VIDIOC_G_PRIORITY:
		*(u32 *)arg = v4l2_prio_max(stream->vdev->prio);
		break;
	case VIDIOC_S_PRIORITY:
		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;
		ret = v4l2_prio_change(stream->vdev->prio, &handle->vfh.prio,
				*(u32 *)arg);
		break;
	case VIDIOC_ENUM_FMT:
		ret = rtscam_video_enum_fmt(stream, arg);
		break;
	case VIDIOC_G_FMT:
		ret = rtscam_video_get_fmt(stream, arg);
		break;
	case VIDIOC_TRY_FMT:
		ret = rtscam_video_try_fmt(stream, arg);
		break;
	case VIDIOC_S_FMT:
		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if ((ret = rtscam_acquire_privileges(handle)) < 0)
			return ret;

		ret = rtscam_video_set_fmt(stream, arg);
		if (ret)
			rtscam_dismiss_privileges(handle);
		break;
	case VIDIOC_ENUM_FRAMESIZES:
		ret = rtscam_video_enum_framesizes(stream, arg);
		break;
	case VIDIOC_ENUM_FRAMEINTERVALS:
		ret = rtscam_video_enum_frameintervals(stream, arg);
		break;
	case VIDIOC_G_PARM:
		ret = rtscam_video_get_parm(stream, arg);
		break;
	case VIDIOC_S_PARM:
		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if ((ret = rtscam_acquire_privileges(handle)) < 0)
			return ret;

		ret = rtscam_video_set_parm(stream, arg);
		if (ret)
			rtscam_dismiss_privileges(handle);
		break;
	case VIDIOC_ENUMINPUT:
		ret = rtscam_video_enum_input(stream, arg);
		break;
	case VIDIOC_G_INPUT:
		ret = rtscam_video_get_input(stream, arg);
		break;
	case VIDIOC_S_INPUT:
		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if ((ret = rtscam_acquire_privileges(handle)) < 0)
			return ret;

		ret = rtscam_video_set_input(stream, *(unsigned int *)arg);
		if (ret)
			rtscam_dismiss_privileges(handle);
		break;
	case VIDIOC_QUERYCTRL:
	case VIDIOC_G_CTRL:
	case VIDIOC_S_CTRL:
	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS:
	case VIDIOC_TRY_EXT_CTRLS:
		ret = rtscam_video_do_ctrl_ioctl(icd, cmd, arg);
		break;
	case VIDIOC_REQBUFS:
	{
		struct v4l2_requestbuffers *p = arg;

		if (!p || p->memory != V4L2_MEMORY_MMAP)
			return -EINVAL;

		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if ((ret = rtscam_acquire_privileges(handle)) < 0)
			return ret;

		mutex_lock(&stream->stream_lock);
		ret = rtscam_video_alloc_buffers(stream, file, p);
		mutex_unlock(&stream->stream_lock);
		if (ret) {
			rtscam_dismiss_privileges(handle);
			return ret;
		}

		if (p->count  == 0)
			rtscam_dismiss_privileges(handle);
		else
			stream->memory_owner = file;

		break;
	}
	case VIDIOC_QUERYBUF:
		if (!rtscam_has_privileges(handle))
			return -EBUSY;

		ret = rtscam_video_query_buffer(stream, arg);
		break;
	case VIDIOC_QBUF:
		if (!rtscam_has_privileges(handle))
			return -EBUSY;

		ret = rtscam_video_queue_buffer(stream, arg);
		break;
	case VIDIOC_DQBUF:
		if (!rtscam_has_privileges(handle))
			return -EBUSY;

		ret = rtscam_video_dequeue_buffer(stream, arg,
				file->f_flags & O_NONBLOCK);
		break;
	case VIDIOC_STREAMON:
	{
		int *type = arg;

		if (*type != stream->vb2_vidp.type)
			return -EINVAL;

		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if (!rtscam_has_privileges(handle))
			return -EBUSY;

		mutex_lock(&icd->dev_lock);

		mutex_lock(&stream->stream_lock);
		ret = rtscam_video_enable(stream, 1);
		mutex_unlock(&stream->stream_lock);

		mutex_unlock(&icd->dev_lock);

		break;
	}
	case VIDIOC_STREAMOFF:
	{
		int *type = arg;

		if (*type != stream->vb2_vidp.type)
			return -EINVAL;

		ret = v4l2_prio_check(stream->vdev->prio, handle->vfh.prio);
		if (ret < 0)
			return ret;

		if (!rtscam_has_privileges(handle))
			return -EBUSY;

		mutex_lock(&icd->dev_lock);

		mutex_lock(&stream->stream_lock);
		ret = rtscam_video_enable(stream, 0);
		mutex_unlock(&stream->stream_lock);

		mutex_unlock(&icd->dev_lock);

		break;
	}
	case VIDIOC_ENUMSTD:
	case VIDIOC_QUERYSTD:
	case VIDIOC_G_STD:
	case VIDIOC_S_STD:
	case VIDIOC_OVERLAY:
	case VIDIOC_ENUMAUDIO:
	case VIDIOC_ENUMAUDOUT:
	case VIDIOC_ENUMOUTPUT:
	case VIDIOC_LOG_STATUS:
	case VIDIOC_SUBSCRIBE_EVENT:
	case VIDIOC_UNSUBSCRIBE_EVENT:
	case VIDIOC_CROPCAP:
	case VIDIOC_G_CROP:
	case VIDIOC_S_CROP:
	case VIDIOC_G_SELECTION:
	case VIDIOC_S_SELECTION:
		rtsprintk(RTS_TRACE_IOCTL,
			"Unsupport [Analog video standards] ioctl 0x%08x\n",
			cmd);
		ret = -ENOTTY;
		break;
	case RTSCAMIOC_VENDOR_CMD:
		ret = rtscam_call_video_op(icd, exec_command, stream, arg);
		break;
	case RTSCAMIOC_GET_PHYADDDR:
	{
		unsigned long vm_addr = *(unsigned long *)arg;
		unsigned long phy_addr = 0;

		ret = rtscam_convert_addr_from_vm_to_phy(icd,
				vm_addr, &phy_addr);

		if (!ret)
			*(unsigned long *)arg = phy_addr;
		else
			rtsprintk(RTS_TRACE_ERROR,
					"<0x%lx> get dma addr fail\n", vm_addr);
		break;
	}
	default:
		rtsprintk(RTS_TRACE_ERROR,
			"Unknown[video] ioctl 0x%08x, type = '%c' nr = 0x%x\n",
			cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		ret = -ENOTTY;
		break;
	}
	rtsprintk(RTS_TRACE_DEBUG,
		"[video] ioctl 0x%08x, type = '%c' nr = 0x%x (%d)\n",
		cmd, _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_NR(cmd));

	return ret;
}

static long rtscam_video_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	return video_usercopy(file, cmd, arg, rtscam_video_do_ioctl);
}

static int rtscam_video_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct rtscam_video_stream *stream = video_drvdata(file);
	int err;

	if (file != stream->memory_owner)
		return -EINVAL;

	if (mutex_lock_interruptible(&stream->stream_lock))
		return -ERESTARTSYS;

	err = rtscam_video_queue_mmap(stream, vma);

	mutex_unlock(&stream->stream_lock);

	return 0;
}

static unsigned int rtscam_video_poll(struct file *file, poll_table *pt)
{
	struct rtscam_video_stream *stream = video_drvdata(file);
	unsigned int err = POLLERR;

	mutex_lock(&stream->stream_lock);
	err = rtscam_video_queue_poll(stream, file, pt);
	mutex_unlock(&stream->stream_lock);

	return err;
}

static struct v4l2_file_operations rtscam_video_fops = {
	.owner = THIS_MODULE,
	.open = rtscam_video_open,
	.release = rtscam_video_close,
	.unlocked_ioctl = rtscam_video_ioctl,
	.mmap = rtscam_video_mmap,
	.poll = rtscam_video_poll,
};

static int video_dev_create(struct rtscam_video_stream *stream)
{
	struct video_device *vdev = video_device_alloc();
	int nr = stream->icd->video_nr;
	int ret;

	if (!vdev)
		return -ENOMEM;

	ret = rtscam_check_stream_format(stream);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
				"please init stream format first\n");
		return ret;
	}
	ret = rtscam_check_user_format(stream, stream->user_format,
			stream->user_width, stream->user_height,
			stream->user_numerator, stream->user_denominator);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
				"please init user format first\n");
		return ret;
	}

	strlcpy(vdev->name, stream->icd->drv_name, sizeof(vdev->name));

	vdev->v4l2_dev = &stream->icd->v4l2_dev;
	vdev->fops = &rtscam_video_fops;
	vdev->release = video_device_release;
	vdev->prio = &stream->prio;
	set_bit(V4L2_FL_USE_FH_PRIO, &vdev->flags);

	video_set_drvdata(vdev, stream);

	ret = video_register_device(vdev, VFL_TYPE_GRABBER, nr);
	if (ret < 0) {
		rtsprintk(RTS_TRACE_ERROR,
			"register video device %d fail, %d\n", nr, ret);
		video_device_release(vdev);
		return ret;
	}

	ret = rtscam_video_init_videobuf2(&stream->vb2_vidp, stream);
	if (ret < 0) {
		rtsprintk(RTS_TRACE_ERROR,
			"video device %d init vb2 fail, %d\n", nr, ret);
		video_unregister_device(vdev);
		return ret;
	}

	stream->vdev = vdev;

	return 0;
}

static int rtscam_video_device_remove(struct rtscam_video_device *icd)
{
	int i;

	rtscam_video_release_ctrl(icd);

	for (i = 0; i < icd->streamnum; i++) {
		struct rtscam_video_stream *stream = icd->streams + i;
		if (!stream->vdev)
			continue;
		rtscam_video_release_videobuf2(&stream->vb2_vidp, stream);
		video_unregister_device(stream->vdev);
		stream->vdev = NULL;
	}

	return 0;
}

static int rtscam_video_device_probe(struct rtscam_video_device *icd)
{
	int ret = 0;
	int i;
	int num = 0;

	rtsprintk(RTS_TRACE_VIDEO,
			"Probing %s\n", dev_name(icd->v4l2_dev.dev));

	if (!icd->drv_name)
		return -EINVAL;

	/*init ctrls*/
	ret = rtscam_video_init_ctrl(icd);
	if (ret < 0) {
		rtsprintk(RTS_TRACE_ERROR, "Init ctrls fail\n");
		return ret;
	}

	if (0 == icd->streamnum || NULL == icd->streams) {
		rtscam_video_release_ctrl(icd);
		rtsprintk(RTS_TRACE_ERROR, "No stream found in icd\n");
		return -EINVAL;
	}

	for (i = 0; i < icd->streamnum; i++) {
		struct rtscam_video_stream *stream = icd->streams + i;

		stream->icd = icd;
		stream->vdev = NULL;

		mutex_init(&stream->stream_lock);

		v4l2_prio_init(&stream->prio);

		atomic_set(&stream->active, 0);
		atomic_set(&stream->use_count, 0);

		ret = video_dev_create(stream);
		if (ret < 0)
			continue;
		num++;
	}
	if (0 == num) {
		ret = -EINVAL;
		goto error;
	}

	return 0;

error:
	rtscam_video_device_remove(icd);

	return ret;
}

int rtscam_video_register_device(struct rtscam_video_device *icd)
{
	int ret = 0;

	if (!icd)
		return -EINVAL;

	if (!icd->alloc_ctx || !icd->mem_ops)
		return -EINVAL;

	if (!icd->initialized)
		return -EINVAL;

	if (icd->video_nr < 0 || icd->video_nr > 64)
		icd->video_nr = -1;

	icd->v4l2_dev.dev = icd->dev;
	ret = v4l2_device_register(icd->v4l2_dev.dev, &icd->v4l2_dev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
				"%s:v4l2_device_register fail\n", __func__);
		return ret;
	}

	atomic_set(&icd->use_count, 0);
	icd->streaming_count = 0;

	ret = rtscam_video_device_probe(icd);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
				"video device <%s> probe fail : %d\n",
				icd->dev_name, ret);
		v4l2_device_unregister(&icd->v4l2_dev);
	} else {
		rtsprintk(RTS_TRACE_INFO,
				"video device <%s> registered\n",
				icd->dev_name);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(rtscam_video_register_device);

int rtscam_video_unregister_device(struct rtscam_video_device *icd)
{
	rtscam_video_device_remove(icd);

	v4l2_device_unregister(&icd->v4l2_dev);

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_video_unregister_device);

