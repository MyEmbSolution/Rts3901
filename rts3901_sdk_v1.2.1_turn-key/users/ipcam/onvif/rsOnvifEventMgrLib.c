/*#include <semaphore.h>*/
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>
#include <errno.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <octopus/octopus.h>
#include <octopus/rts_cam.h>

#include "rsOnvifDefines.h"
#include "rsOnvifEventMgrLib.h"
#include "rsOnvifCommonFunc.h"


#define SUBMGR_SHARED_MEMORY_NAME "/onvif_events_submgr_shm"
#define SUBMGR_SEMAPHORE_NAME "/bin"

#define MESSAGE_PRIORITY 2

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

static int g_submgr_sem_id = -1;

union semun {
	int  val; /* Value for SETVAL */
	struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
	struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux specific) */
};

static int rs_sysv_sem_create(void)
{
	int sem_id;

	/* Generate a System V IPC key */
	key_t key;
	key = ftok(SUBMGR_SEMAPHORE_NAME, 0xD4);
	if (key == -1) {
		RS_DBG("Semaphore ftok() failed !! [%s]\n",
			strerror(errno));
		return -1;
	}

	/* Get a semaphore set with 1 semaphore */
	sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
	if (sem_id == -1) {
		if (errno == EEXIST) {
			sem_id = semget(key, 1, 0666);
			if (sem_id == -1) {
				RS_DBG("Semaphore semget() failed !! [%s]\n",
					strerror(errno));
				return -1;
			}
		} else {
			RS_DBG("Semaphore semget() failed !! [%s]\n",
				strerror(errno));
			return -1;
		}
	} else {
		/* Initialize semaphore #0 to 1 */
		union semun arg;

		arg.val = 1;
		if (semctl(sem_id, 0, SETVAL, arg) == -1) {
			RS_DBG("Semaphore semctl() failed !! [%s]\n",
				strerror(errno));
			return -1;
		}
	}

	g_submgr_sem_id = sem_id;
	return 0;
}

static int rs_sysv_sem_lock(void)
{
	struct sembuf sop[1];

	if (g_submgr_sem_id == -1) {
		if (rs_sysv_sem_create() == -1) {
			RS_DBG("rs_sysv_sem_create fail\n");
			return -1;
		}
	}

	sop[0].sem_num = 0;
	sop[0].sem_op = -1;
	sop[0].sem_flg = SEM_UNDO;

try_again:
	if (semop(g_submgr_sem_id, sop, 1) == -1) {
		if (errno == EINTR) {
			goto try_again;
		}
		RS_DBG("Semaphore Lock semop() failed !! [%s]\n",
			strerror(errno));
		return -1;
	}
	return 0;
}

static int rs_sysv_sem_unlock(void)
{
	struct sembuf sop[1];

	sop[0].sem_num = 0;
	sop[0].sem_op = 1;
	sop[0].sem_flg = SEM_UNDO;
	if (semop(g_submgr_sem_id, sop, 1) == -1) {
		RS_DBG("Semaphore Unlock semop() failed !! [%s]\n",
			strerror(errno));
		return -1;
	}
	return 0;
}


static int rsSubmgrDataLock(void **ptr, int *shmfd)
{
	int ret = 0;
	/*shared memory*/
	*shmfd = shm_open(SUBMGR_SHARED_MEMORY_NAME, O_RDWR, 0666);
	if (*shmfd < 0) {
		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		return 1;
	}

	*ptr = (void *)mmap(NULL, sizeof(submgr_info_t),
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				*shmfd,
				0);
	if (*ptr == NULL) {

		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		return 2;
	}

	ret = rs_sysv_sem_lock();
	if (ret) {
		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		munmap(*ptr, sizeof(submgr_info_t));
		close(*shmfd);
		return 3;
	}

	return 0;
}

static int rsSubmgrDataUnlock(void *ptr, int shmfd)
{
	int ret = 0;
	ret = rs_sysv_sem_unlock();
	if (ret) {
		RS_DBG("Semaphore failed to unlock\n");
		return ret;
	}
	munmap(ptr, sizeof(submgr_info_t));
	close(shmfd);
	sched_yield();
	return 0;
}

int rs_onvif_events_submgr_info_create()
{
	int shmfd;
	int ret = 0;
	submgr_info_t *info;
	mode_t old_mode;


	old_mode = umask(0);

	shmfd = shm_open(SUBMGR_SHARED_MEMORY_NAME,
			O_CREAT | O_EXCL | O_RDWR,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (shmfd < 0) {
		if (errno == EEXIST) {
			umask(old_mode);
			return 0;
		} else {
			return 1;
		}
	}

	umask(old_mode);

	ftruncate(shmfd, sizeof(submgr_info_t));

	info = (submgr_info_t *)mmap(NULL, sizeof(submgr_info_t),
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					shmfd, 0);
	if (info == NULL) {

		RS_DBG("failed to mmap\n");
		shm_unlink(SUBMGR_SHARED_MEMORY_NAME);
		return 2;
	}

	memset(info, 0, sizeof(submgr_info_t));

	ret = rs_sysv_sem_create();
	if (ret) {
		RS_DBG("unable to create semaphore\n");
		munmap(info, sizeof(submgr_info_t));
		return 3;
	}

	ret = rs_sysv_sem_lock();
	if (ret) {
		RS_DBG("Semaphore failed to lock\n");
		munmap(info, sizeof(submgr_info_t));
		return 3;
	}

	info->submgr_number = 0;
	RS_DBG("In func %s submgr_number = %d\n", __func__, info->submgr_number);

	ret = rs_sysv_sem_unlock();
	if (ret) {
		RS_DBG("Semaphore failed to unlock\n");
		return ret;
	}

	munmap((void *)info, sizeof(submgr_info_t));
	close(shmfd);
	return 0;
}

int rs_onvif_events_submgr_info_destroy()
{
	shm_unlink(SUBMGR_SHARED_MEMORY_NAME);
	return 0;
}

int rs_onvif_events_submgr_info_get_submgr_number(int *number)
{
	submgr_info_t *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSubmgrDataLock((void **)&local_info, &shmfd);
	if (ret) {
		RS_DBG("failed to lock\n");
		return ret;
	}

	RS_DBG("submgr_num:%d\n", local_info->submgr_number);
	*number =  local_info->submgr_number;
	RS_DBG("before unlock\n");
	rsSubmgrDataUnlock(local_info, shmfd);
	RS_DBG("after unlock\n");
	return 0;
}


int rs_onvif_events_submgr_info_put_item(events_submgr_info_item_t *item)
{
	RS_DBG("In func %s \n", __func__);
	submgr_info_t *local_info;
	int shmfd;
	int ret = 0;
	int i;
	ret = rsSubmgrDataLock((void **)&local_info, &shmfd);
	if (ret) {
		RS_DBG("failed to lock\n");
		return ret;
	}

	memcpy(local_info->items[local_info->submgr_number].id, item->id, SUBMGR_ID_BUFFER_LEN);
	local_info->items[local_info->submgr_number].type = item->type;
	local_info->submgr_number++;
	RS_DBG("In func %s submgr_number = %d\n", __func__, local_info->submgr_number);
	rsSubmgrDataUnlock(local_info, shmfd);
	return 0;
}

int rs_onvif_events_submgr_info_remove_item(char *submgr_id)
{
	RS_DBG("In func %s\n", __func__);
	submgr_info_t *local_info;
	int shmfd;
	int ret = 0;
	int i, j;
	ret = rsSubmgrDataLock((void **)&local_info, &shmfd);
	if (ret) {
		RS_DBG("failed to lock\n");
		return ret;
	}

	for (i = 0; i < local_info->submgr_number; i++) {
		if (!strncmp(local_info->items[i].id, submgr_id, strlen(local_info->items[i].id))) {
			for (j = i; j < (local_info->submgr_number - 1); j++) {
				memcpy(local_info->items[j].id, local_info->items[j + 1].id, SUBMGR_ID_BUFFER_LEN);
				local_info->items[j].type = local_info->items[j+1].type;
			}
			memset(local_info->items[local_info->submgr_number - 1].id, 0, SUBMGR_ID_BUFFER_LEN);
			local_info->items[local_info->submgr_number - 1].type = -1;
			break;
		}
	}

	local_info->submgr_number--;

	RS_DBG("In func %s submgr_number = %d\n", __func__, local_info->submgr_number);
	rsSubmgrDataUnlock(local_info, shmfd);
	return 0;
}

int rs_onvif_events_submgr_unsubscribe(events_submgr_unscribe_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	events_submgr_message *msg;
	char mq_name[50] = {0};
	sprintf(mq_name, "%s%s", SUBMGR_MSGQ_NAME_PREFIX, arg->submgr_id);
	msgq_id = mq_open(mq_name, O_RDWR);
	RS_DBG("msgq_id %d, errno %d\n", msgq_id, errno);
	if ((mqd_t)-1 == msgq_id) {
		return 1;
	}
	ASSERT(sizeof(events_submgr_unscribe_t) <= (sizeof(events_submgr_message) - 4));
	msg = (events_submgr_message *)malloc(sizeof(events_submgr_message));
	ASSERT(msg);

	msg->type = EVENTS_SUBMGR_MESSAGE_UNSUBSCRIBE;
	memcpy(msg->data, arg, sizeof(events_submgr_unscribe_t));
	ret = mq_send(msgq_id, (char *)msg, sizeof(events_submgr_message), MESSAGE_PRIORITY);
	RS_DBG("ret %d, errno %d\n", ret, errno);
	mq_close(msgq_id);
	free(msg);
	return ret;
}

int rs_onvif_events_submgr_renew(events_submgr_renew_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	events_submgr_message *msg;
	char mq_name[50] = {0};
	sprintf(mq_name, "%s%s", SUBMGR_MSGQ_NAME_PREFIX, arg->submgr_id);
	msgq_id = mq_open(mq_name, O_RDWR);
	if ((mqd_t)-1 == msgq_id) {
		return 1;
	}
	ASSERT(sizeof(events_submgr_renew_t) <= (sizeof(events_submgr_message) - 4));
	msg = (events_submgr_message *)malloc(sizeof(events_submgr_message));
	ASSERT(msg);

	msg->type = EVENTS_SUBMGR_MESSAGE_RENEW;
	memcpy(msg->data, arg, sizeof(events_submgr_renew_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(events_submgr_message), MESSAGE_PRIORITY);
	mq_close(msgq_id);
	free(msg);
	return ret;
}

int rs_motion_detect(const char *dev_path)
{
	int ret = 0, i = 0;
	int fd = 0;
	int tmp_result = -1;
	if (!dev_path)
		dev_path = "/dev/video51";

	fd = rts_cam_open(dev_path);
	if (fd < 0)
		return -1;

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		return -2;
	}

	struct attr_md *p_attr_md = NULL;
	ret = rts_cam_get_md_attr(fd, &p_attr_md);
	for (i = 0; i < p_attr_md->number; i++) {
		if (!p_attr_md->blocks[i].enable)
			goto out;
	}
	ret = rts_cam_set_md_attr(fd, p_attr_md);
	uint32_t property = 0;
	rts_cam_get_events(fd, EVENT_MOTION_DETECT, &property);
	tmp_result = property & 0xFF;
out:
	rts_cam_free_attr(p_attr_md);
	rts_cam_unlock(fd);
	rts_cam_close(fd);
	return tmp_result;
}
