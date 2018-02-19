#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <mqueue.h>
#include <stdio.h>
#include "soapH.h"
/*#include "PullPointSubscriptionBinding.nsmap"*/
#include "rsOnvifEventMgrLib.h"
#include "rsOnvifTypes.h"

#define SUBMGR_MSGQ_LEN 10

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

static int create_message_queue(char *submgr_id, mqd_t *msgq_id)
{
	int ret = 0;
	struct mq_attr attr;
	char mq_name[50] = {0};
	mode_t old_mode;

	sprintf(mq_name, "%s%s", SUBMGR_MSGQ_NAME_PREFIX, submgr_id);
	RS_DBG("dynamic msg queue name = %s\n", mq_name);

	/*
	ret = mq_unlink(mq_name);
	if (!ret) {
		RS_DBG("failed to unlink %s errno %d\n", mq_name, errno);
	}
	*/

	RS_DBG("init attributes\n");
	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = SUBMGR_MSGQ_LEN;
	attr.mq_msgsize = sizeof(events_submgr_message);
	attr.mq_curmsgs = 0;

	RS_DBG("init attributes 1\n");
	/* create the message queue */
	old_mode = umask(0);
	RS_DBG("after umask\n");
	*msgq_id = mq_open(mq_name, O_CREAT|O_EXCL|O_RDWR, 0666, &attr);
	RS_DBG("mq_open ret = %d\n", *msgq_id);
	if ((mqd_t)-1 == *msgq_id) {
		ret = 1;
		RS_DBG("create msgqueue failed errno %d\n", errno);
		RS_DBG("ret id = %d\n", *msgq_id);
	}
	RS_DBG("after umask 2\n");
	umask(old_mode);

	RS_DBG("ret = %d\n", ret);
	return ret;
}

static int delete_message_queue(char *submgr_id, mqd_t msgq_id)
{
	int ret = 0;
	char mq_name[50] = {0};
	mode_t old_mode;

	sprintf(mq_name, "%s%s", SUBMGR_MSGQ_NAME_PREFIX, submgr_id);
	mq_close(msgq_id);
	ret = mq_unlink(mq_name);

	return ret;
}

static void send_notification(char *consumer_address)
{
#if 1
	RS_DBG("In func\n");
	int ret = 0;
	time_t tnow;
	struct soap *soap = soap_new();
	char *response_message = (char *)soap_malloc(soap, 1024);

	int motion_detect_result = rs_motion_detect(NULL);
	char tmpbuf[64] = {0};
	time(&tnow);
	strftime(tmpbuf, sizeof(tmpbuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&tnow));
	sprintf(response_message, MOTION_DETECT_TEMPLATE,
		tmpbuf,
		(motion_detect_result & 0x02) >> 1,
		(motion_detect_result & 0x04) >> 2,
		(motion_detect_result & 0x08) >> 3
		);


	struct _wsnt__Notify *notify = (struct _wsnt__Notify *)soap_malloc(soap, sizeof(struct _wsnt__Notify));

	memset(notify, 0, sizeof(struct _wsnt__Notify));

	soap_init1(soap, SOAP_IO_CHUNK);

	notify->__sizeNotificationMessage = 1;
	notify->NotificationMessage =
		(struct wsnt__NotificationMessageHolderType *)soap_malloc(soap,
			sizeof(struct wsnt__NotificationMessageHolderType) * 1);
	notify->NotificationMessage[0].SubscriptionReference = NULL;
	notify->NotificationMessage[0].ProducerReference = NULL;

	notify->NotificationMessage[0].Topic =
		(struct wsnt__TopicExpressionType *)soap_malloc(soap,
			sizeof(struct wsnt__TopicExpressionType));
	/*
	 notify->NotificationMessage[0].Topic->__any = "tns1:VideoAnalytics/tns1:MotionDetection/tns1:Motion";
	 */
	 notify->NotificationMessage[0].Topic->__any = "tns1:VideoAnalytics/tnsavg:MotionDetection/tns1:Motion";

	/*
	notify->NotificationMessage[0].Topic->__any = "tns1:VideoAnalytics/tnsavg:MotionDetection";
	*/
	notify->NotificationMessage[0].Topic->Dialect = "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet";
	notify->NotificationMessage[0].Topic->__anyAttribute = NULL;
	notify->NotificationMessage[0].Topic->__mixed = NULL;


	notify->NotificationMessage[0].Message.__any = response_message;
	notify->__size = 0;
	notify->__any = NULL;
	ret = soap_send___tev__Notify(soap, consumer_address, NULL, notify);
	RS_DBG("\nsoap_send___tev__Notify return value: %d,soap version %d\n", ret, soap->version);
	soap_destroy(soap);
	soap_end(soap);
	soap_done(soap);
	free(soap);
#endif
}

static int notification_loop(events_submgr_parms_t *arg, mqd_t msgq)
{
	RS_DBG("In func\n");
	long timeout = arg->timeout;
	time_t start, current, end;
	struct timespec abs_timeout;
	events_submgr_message *msg;

	msg = (events_submgr_message *)malloc(sizeof(events_submgr_message));
	if (msg == NULL) {
		RS_DBG("failed to malloc msg buffer exit\n");
		return 1;
	}

	time(&start);
	end = start + timeout;
	send_notification(arg->consumer_address);
	while (1) {

		ssize_t bytes_read;

		abs_timeout.tv_sec = end;
		abs_timeout.tv_nsec = 0;

		/* receive the message */
		bytes_read = mq_timedreceive(msgq, (char *)msg, sizeof(events_submgr_message), NULL, &abs_timeout);
		ASSERT(bytes_read != -1 || errno == ETIMEDOUT);
		RS_DBG("bytes_read %d\n", bytes_read);

		time(&current);
		/*process the message*/
		if (msg->type == EVENTS_SUBMGR_MESSAGE_UNSUBSCRIBE) {
			RS_DBG("Unsubscribe command arrived!\n");
			break;
		} else if (msg->type == EVENTS_SUBMGR_MESSAGE_RENEW) {
			/*reset the end value*/
			events_submgr_renew_t *arg = (events_submgr_renew_t *)msg->data;
			end = current + arg->time_out;
			RS_DBG("Renew command arrived!, time_out %d\n", arg->time_out);
		}

		if (current  >= end) {
			break;
		}
	}

	RS_DBG("start %ld, timeout %ld, current %ld\n",
		start, timeout, current);
}


static int pullpoint_loop(events_submgr_parms_t *arg, mqd_t msgq)
{
	RS_DBG("In func\n");
	long timeout = arg->timeout;
	time_t start, current, end;
	struct timespec abs_timeout;
	events_submgr_message *msg;

	msg = (events_submgr_message *)malloc(sizeof(events_submgr_message));
	if (msg == NULL) {
		RS_DBG("failed to malloc msg buffer exit\n");
		return 1;
	}

	time(&start);
	end = start + timeout;
	while (1) {

		ssize_t bytes_read;

		abs_timeout.tv_sec = end;
		abs_timeout.tv_nsec = 0;

		/* receive the message */
		bytes_read = mq_timedreceive(msgq, (char *)msg, sizeof(events_submgr_message), NULL, &abs_timeout);
		ASSERT(bytes_read != -1 || errno == ETIMEDOUT);
		RS_DBG("bytes_read %d\n", bytes_read);

		time(&current);
		/*process the message*/
		if (msg->type == EVENTS_SUBMGR_MESSAGE_UNSUBSCRIBE) {
			RS_DBG("Unsubscribe command arrived!\n");
			break;
		} else if (msg->type == EVENTS_SUBMGR_MESSAGE_RENEW) {
			/*reset the end value*/
			events_submgr_renew_t *arg = (events_submgr_renew_t *)msg->data;
			end = current + arg->time_out;
			RS_DBG("Renew command arrived!, time_out %d\n", arg->time_out);
		}

		if (current  >= end) {
			break;
		}
	}

	RS_DBG("exit func start %ld, timeout %ld, current %ld\n",
		start, timeout, current);
}


/*subscription manager*/
void rs_onvif_events_submgr(events_submgr_parms_t *arg)
{
	events_submgr_info_item_t item;
	int ret = 0, submgr_cnt;
	mqd_t msgq;

	RS_DBG("In func\n");

	ret = rs_onvif_events_submgr_info_create();
	ASSERT(!ret);
	/*
	strncpy(item.id, arg->id, SUBMGR_ID_BUFFER_LEN);
	item.type = arg->type;
	ret = rs_onvif_events_submgr_info_put_item(&item);
	ASSERT(!ret);
	*/

	ret = create_message_queue(arg->id, &msgq);
	if (1 == ret) {
		RS_DBG("create msg queue failed\n");
	}
	ASSERT(!ret);

	strncpy(item.id, arg->id, SUBMGR_ID_BUFFER_LEN);
	item.type = arg->type;
	ret = rs_onvif_events_submgr_info_put_item(&item);
	ASSERT(!ret);


	RS_DBG("type %d\n", arg->type);

	if (arg->type == SUBMGR_TYPE_NOTIFICATION) {
		notification_loop(arg, msgq);
	} else {
		pullpoint_loop(arg, msgq);
	}

	ret = delete_message_queue(arg->id, msgq);
	ASSERT(!ret);

	ret = rs_onvif_events_submgr_info_remove_item(arg->id);
	ASSERT(!ret);

	ret = rs_onvif_events_submgr_info_get_submgr_number(&submgr_cnt);
	ASSERT(!ret);
	RS_DBG("submgr_cnt %d\n", submgr_cnt);
	if (!submgr_cnt) {
		rs_onvif_events_submgr_info_destroy();
	}
	return;
}

int main(int argc, void **argv)
{
	char log_file[200];
	events_submgr_parms_t arg;
	RS_DBG("************ +++++++++++ *************************\n");
	RS_DBG("submanager start!\n");
	if (argc != 5) {
		RS_DBG("argc is not 5 but %d\n", argc);
		RS_DBG("usage:\nevents_submgr type id timout notify_target_address\n");
		ASSERT(0);
	}
	arg.type = (submgr_type_t)atoi(argv[1]);
	arg.id = (char *)argv[2];
	arg.timeout = atoi(argv[3]);
	arg.consumer_address = (char *)argv[4];

	RS_DBG("id %s, timout %d, consumer_address %s\n", arg.id, arg.timeout, arg.consumer_address);

	rs_onvif_events_submgr(&arg);
	RS_DBG("************ ------------ *************************\n");
}
