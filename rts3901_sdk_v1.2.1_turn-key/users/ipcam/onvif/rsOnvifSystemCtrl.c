#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <mqueue.h>
#include <pthread.h>
#include <fcntl.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>     /* gethostbyname */
#include <unistd.h>
#include <linux/reboot.h>

#include <rtsnm.h>
#include <auth/auth.h>

#include "soapH.h"
#include "wsddapi.h"

#include "rsOnvifDefines.h"
#include "rsOnvifTypes.h"
#include "rsOnvifCommonFunc.h"
#include "rsOnvifConfig.h"
#include "rsOnvifSystemCtrl.h"
#include "rsOnvifRWJsonConfig.h"


/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

static int g_sem_id = -1;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO(Linux specific) */
};

static int rs_sysv_sem_create(void)
{
	int sem_id;

	/* Generate a System V IPC key */
	key_t key;
	key = ftok(SYSTEM_SEMAPHORE_NAME, 0xD4);
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

	g_sem_id = sem_id;
	return 0;
}

static int rs_sysv_sem_lock(void)
{
	struct sembuf sop[1];

	if (g_sem_id == -1) {
		if (rs_sysv_sem_create() == -1) {
		    RS_DBG("rs_sysv_sem_create fail\n");
			return -1;
		}
	}

	sop[0].sem_num = 0;
	sop[0].sem_op = -1;
	sop[0].sem_flg = SEM_UNDO;

try_again:
	if (semop(g_sem_id, sop, 1) == -1) {
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
	if (semop(g_sem_id, sop, 1) == -1) {
		RS_DBG("Semaphore Unlock semop() failed !! [%s]\n",
			strerror(errno));
		return -1;
	}
	return 0;
}

/*static int rsSystemProcessLock(void **ptr, int *shmfd, sem_t **mutex)*/
static int rsSystemProcessLock(void **ptr, int *shmfd)
{
	int ret = 0;
	/*shared memory*/
	*shmfd = shm_open(SYSTEM_SHARED_MEMORY_NAME, O_RDWR, 0666);
	if (*shmfd < 0) {
		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		return 1;
	}

	*ptr = (void *)mmap(NULL, sizeof(RS_LOCAL_SYSTEM_INFO),
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				*shmfd,
				0);
	if (*ptr == NULL) {

		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		return 2;
	}

	/*mutex*/
	/*
	*mutex = sem_open(SYSTEM_SEMAPHORE_NAME, 0, 0666, 0);
	if (SEM_FAILED == *mutex) {

		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		munmap(*ptr, sizeof(RS_LOCAL_SYSTEM_INFO));
		close(*shmfd);
		return 3;
	}

	if (sem_wait(*mutex) != 0) {

		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		sem_close(*mutex);
		munmap(*ptr, sizeof(RS_LOCAL_SYSTEM_INFO));
		close(*shmfd);
		return 4;
	}

	*/

	ret = rs_sysv_sem_lock();
	if (ret) {
		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,\\n",
			__FILE__, __func__, __LINE__, errno);
		munmap(*ptr, sizeof(RS_LOCAL_SYSTEM_INFO));
		close(*shmfd);
		return 3;
	}

	return 0;
}

/*static int rsSystemProcessUnlock(void *ptr, int shmfd, sem_t *mutex)*/
static int rsSystemProcessUnlock(void *ptr, int shmfd)
{
	int ret = 0;
	/*
	if (sem_post(mutex)!=0) {
		RS_DBG("Error in File :%s,Func:%s,Line:%d,error id:%d,failed to post semaphore\\n",
			__FILE__, __func__, __LINE__, errno);
	}
	sem_close(mutex);
	*/
	ret = rs_sysv_sem_unlock();
	if (ret) {
		RS_DBG("Semaphore failed to unlock\n");
		return ret;
	}
	munmap(ptr, sizeof(RS_LOCAL_SYSTEM_INFO));
	close(shmfd);
	sched_yield();
	return 0;
}

static int rsFillDefaultSetting(RS_LOCAL_SYSTEM_INFO *info)
{

	int i;
	int ret;
	int protocol_idx = 0;
	RS_System_Info *system_info;
	char *scopes_profile = SCOPES_PROFILE;
	char *scopes_type = SCOPES_TYPE;
	char *scopes_name = SCOPES_NAME;
	char *scopes_location = SCOPES_LOCATION;
	char *scopes_hardware = SCOPES_HARDWARE;
	char *scopes_others = SCOPES_OTHERS;

	char *ns[ONVIF_SERVICE_NUM] = {
		DEVICE_NS,
		MEDIA_NS,
		DEVICEIO_NS,
		PTZ_NS,
		IMAGING_NS,
		ANALYTICSDEVICE_NS,
		EVENTS_NS };

	char *service_name[ONVIF_SERVICE_NUM] = {
		DEVICE_SERVICE_NAME,
		MEDIA_SERVICE_NAME,
		DEVICEIO_SERVICE_NAME,
		PTZ_SERVICE_NAME,
		IMAGING_SERVICE_NAME,
		ANALYTICSDEVICE_SERVICE_NAME,
		EVENTS_SERVICE_NAME
	};
	/*service version, (MAJOR<<8)|MINOR;*/
	short service_version[ONVIF_SERVICE_NUM] = {
		DEVICE_SERVICE_VERSION,
		MEDIA_SERVICE_VERSION,
		DEVICEIO_SERVICE_VERSION,
		PTZ_SERVICE_VERSION,
		IMAGING_SERVICE_VERSION,
		ANALYTICSDEVICE_SERVICE_VERSION,
		EVENTS_SERVICE_VERSION
	};

	char network_names[MAX_NETWORK_INTERFACE_NUM][20];

	info->change_counter = 0;
	info->state = 0;
	system_info = &info->system_info;

	rsOnvifSysGetDevUUID(system_info->uuid, UUID_BUF_SIZE);

	/*onvif_speical_info*/
	strncpy(system_info->onvif_special_info.scopes.profile,
		scopes_profile, MAX_SCOPES_BUF_SIZE);
	explodeitem(scopes_type, ' ',
		system_info->onvif_special_info.scopes.type, MAX_SCOPES_TYPE_NUM);

	strncpy(system_info->onvif_special_info.scopes.name,
		scopes_name, MAX_SCOPES_BUF_SIZE);
	explodeitem(scopes_location, ' ',
		system_info->onvif_special_info.scopes.location, MAX_SCOPES_LOCATION_NUM);

	strncpy(system_info->onvif_special_info.scopes.hardware,
		scopes_hardware, MAX_SCOPES_BUF_SIZE);
	explodeitem(scopes_others, ' ',
		system_info->onvif_special_info.scopes.others, MAX_SCOPES_LOCATION_NUM);


	for (i = 0; i < ONVIF_SERVICE_NUM; i++) {
		strncpy(&system_info->onvif_special_info.service_ns[i][0],
			ns[i], ONVIF_SERVICE_NS_LENGTH);
		strncpy(&system_info->onvif_special_info.service_name[i][0],
			service_name[i], ONVIF_SERVICE_NAME_LEGNTH);
		system_info->onvif_special_info.service_version[i] = service_version[i];
	}

	strncpy(system_info->onvif_special_info.device_type,
		DEVICE_TYPE, DEVICE_TYPE_BUF_SIZE);

	memcpy(&system_info->onvif_special_info.device_service_capabilities,
		&device_service_capabilities,
		sizeof(device_service_capabilities_t));

	memcpy(&system_info->onvif_special_info.event_service_capabilities,
		&event_service_capabilities,
		sizeof(event_service_capabilities_t));
	/*
	RS_DBG("In func:%s\n", __func__);
	RS_DBG("\t ws sub policy support = %d\n",event_service_capabilities.ws_subscription_policy_support);
	RS_DBG("\t ws pull point support = %d\n",event_service_capabilities.ws_pull_point_support);
	RS_DBG("\t ws pausable support = %d\n",event_service_capabilities.ws_pausable_submgr_support);
	RS_DBG("\t ws persisten support = %d\n",event_service_capabilities.persistentNotificationStorage);
	RS_DBG("\t ws max producers= %d\n",event_service_capabilities.max_notificationproducers);
	RS_DBG("\t ws max_pull_point = %d\n",event_service_capabilities.max_pull_points);
	*/

	/*device info. The following lines should customized for the different device;*/
	strncpy(system_info->device_info.manufacturer, DEVICE_MANUFACTURER, MID_INFO_SIZE);
	strncpy(system_info->device_info.model, DEVICE_MODEL, MID_INFO_SIZE - 1);
	strncpy(system_info->device_info.firmware_version, DEVICE_FIRMWARE_VERSION, MID_INFO_SIZE);

	rsOnvifSysGetDevSerialNumber(system_info->device_info.serial_number, MID_INFO_SIZE);
	strncpy(system_info->device_info.hardware_id, DEVICE_MODEL, MID_INFO_SIZE);

	system_info->date_time_settings.time_ntp = 0;
	system_info->date_time_settings.day_light_savings = 0;
	strncpy(system_info->date_time_settings.time_zone,
		DEVICE_TIME_ZONE, MAX_TZ_LENGTH);
	/*network configuration;*/
	strncpy(system_info->network_config.system_uris.system_log_uri, SYSTEM_LOG_URI, MID_INFO_SIZE);
	strncpy(system_info->network_config.system_uris.system_backup_uri, SYSTEM_BACKUP_URI, MID_INFO_SIZE);
	strncpy(system_info->network_config.system_uris.support_info_uri, SUPPORT_INFO_URI, MID_INFO_SIZE);
	strncpy(system_info->network_config.system_uris.system_restore_uri, SYSTEM_RESTORE_URI, MID_INFO_SIZE);

	system_info->network_config.discovery_mode = 0;
	system_info->network_config.remote_discovery_mode = 0;
	system_info->network_config.discover_proxy_address[0] = '\0';
	/*only one default account.*/
	strncpy(system_info->network_config.accounts[0].user, DEFAULT_ACCOUNT_NAME, USER_LEN);
	strncpy(system_info->network_config.accounts[0].password, DEFAULT_ACCOUNT_PSWD, PASSWORD_LEN);
	system_info->network_config.accounts[0].authority = DEFAULT_ACCOUNT_LEVEL;
	system_info->network_config.accounts[0].fixed = 1;
	/*should get hostname from the environment;*/

	if (sethostname(system_info->network_config.host_name.name, MID_INFO_SIZE) == -1) {
		strncpy(system_info->network_config.host_name.name, DEFAULT_HOSTNAME, MID_INFO_SIZE);
	}
	system_info->network_config.host_name.dhcp_enable = 0;
	strncpy(system_info->network_config.dns.search_domain[0], DEFAULT_SEARCH_DOMAINNAME, SEARCH_DOMAIN_LENGTH);

	ret = rsOnvifNetGetNetworkInterfaces(network_names,
		&system_info->network_config.interface_num);
	ASSERT(!ret);
	system_info->network_config.interface_idx = 0;
	for (i = 0; i < system_info->network_config.interface_num; i++) {
		strcpy(system_info->network_config.interfaces[i].token, network_names[i]);
		rsOnvifNetGetHWAddr(system_info->network_config.interfaces[i].token,
				system_info->network_config.interfaces[i].mac);
		system_info->network_config.interfaces[i].ip.s_addr =
			rsOnvifNetGetIFAddr(system_info->network_config.interfaces[i].token);
		system_info->network_config.interfaces[i].enabled = 0;
		system_info->network_config.interfaces[i].dhcp_enable = 0;
		system_info->network_config.interfaces[i].mtu = 1500;/* a value needs to adjust;*/
	}

	system_info->network_config.interfaces[system_info->network_config.interface_idx].enabled = 1;


	system_info->network_config.netmask.s_addr =
		rsOnvifNetGetNetmask(
			system_info->network_config.interfaces[system_info->network_config.interface_idx].token);
	system_info->network_config.gateway.s_addr = rsOnvifNetGetGateway();
	rsOnvifNetGetDNS(system_info->network_config.dns.manul_dns, MAX_DNS_NUM);
	system_info->network_config.dns.dhcp_enable = 0;

	system_info->network_config.ntp.dhcp_enable = 0;
	/* should get ntp addr from set_time.json in future*/
	struct hostent *ntpserver;
	ntpserver = gethostbyname(DEFAULT_NTP_ADDRESS);

	if (ntpserver && ntpserver->h_length == 4) {
		memcpy(&(system_info->network_config.ntp.manual_ntp[0].s_addr),
				ntpserver->h_addr_list[0], 4);
	} else {
		inet_aton("0.0.0.0",
			&system_info->network_config.ntp.manual_ntp[0]);
	}

	system_info->network_config.ntp.dhcp_ntp[0].s_addr = 0;

	protocol_idx = 0;
	strcpy(system_info->network_config.protocol[protocol_idx].name, HTTP_NAME);
	system_info->network_config.protocol[protocol_idx].enabled = 1;
	system_info->network_config.protocol[protocol_idx].port[0] = HTTP_PORT;
	protocol_idx++;


	if (system_info->onvif_special_info.device_service_capabilities.security.tls1_x002e0 ||
		system_info->onvif_special_info.device_service_capabilities.security.tls1_x002e1 ||
		system_info->onvif_special_info.device_service_capabilities.security.tls1_x002e2) {
		strcpy(system_info->network_config.protocol[protocol_idx].name, HTTPS_NAME);
		system_info->network_config.protocol[protocol_idx].enabled = 1;
		system_info->network_config.protocol[protocol_idx].port[0] = HTTPS_PORT;
		protocol_idx++;
	}

	strcpy(system_info->network_config.protocol[protocol_idx].name, RTSP_NAME);
	system_info->network_config.protocol[protocol_idx].enabled = 1;
	system_info->network_config.protocol[protocol_idx].port[0] = H264_PORT_1;
	system_info->network_config.protocol[protocol_idx].port[1] = H264_PORT_2;
	system_info->network_config.protocol[protocol_idx].port[2] = MPEG4_PORT_1;
	system_info->network_config.protocol[protocol_idx].port[3] = MPEG4_PORT_2;
	system_info->network_config.protocol[protocol_idx].port[4] = MJPEG_PORT_1;

	/*fill default ip to vec*/
	/*strcpy(default_profile_A.vec.multicast_ip_addr, inet_ntoa(system_info->network_config.interfaces[0].ip));*/
	/*strcpy(default_profile_B.vec.multicast_ip_addr, default_profile_A.vec.multicast_ip_addr);*/
	memcpy(&system_info->media_config,
		&default_media_config, sizeof(media_config_t));
	memcpy(&system_info->media_profile[0],
		&default_profile_A, sizeof(media_profile_t));
	memcpy(&system_info->media_profile[1],
		&default_profile_B, sizeof(media_profile_t));
	/*
	memcpy(&system_info->media_profile[2],
		&default_profile_C, sizeof(media_profile_t));
	*/

	memcpy(&system_info->osd_config[0],
		&default_OSD_A, sizeof(osd_configuration_t));
	memcpy(&system_info->osd_config[1],
		&default_OSD_B, sizeof(osd_configuration_t));

	RS_DBG("Leaving Func %s\n", __func__);
	return 0;
}

static int rsOnvifSysFillSettingsFromMemory(RS_LOCAL_SYSTEM_INFO *info)
{
	int ret = 0;
	FILE *fp;
	int size = 0;
	int bytes_read = 0;
	RS_System_Info *system_info;
	char network_names[MAX_NETWORK_INTERFACE_NUM][20];
	int i;
	int dhcp = 0;
	if (rsOnvifSysInfoReadFromJsonFile(info, SYSTEM_INFO_NAME)) {
		RS_DBG("failed to open %s\nload from default setting\n", SYSTEM_INFO_NAME);
		ret = 1;
		return ret;

	}
	info->change_counter++;
	info->state = 0;
	system_info = &info->system_info;

	ASSERT(!ret);
	for (i = 0; i < system_info->network_config.interface_num; i++) {
		/*all configuration should not be changed but ip address after rebooting;*/
		system_info->network_config.interfaces[i].ip.s_addr =
			rsOnvifNetGetIFAddr(system_info->network_config.interfaces[i].token);
		if (system_info->network_config.interfaces[i].dhcp_enable &&
			system_info->network_config.interfaces[i].enabled) {
			dhcp = 1;
		}
	}

	system_info->network_config.netmask.s_addr =
		rsOnvifNetGetNetmask(
			system_info->network_config.interfaces[system_info->network_config.interface_idx].token);

	system_info->network_config.gateway.s_addr = rsOnvifNetGetGateway();
	if (dhcp) {
		system_info->network_config.dns.dhcp_enable = 1;
		rsOnvifNetGetDNS(system_info->network_config.dns.dhcp_dns, MAX_DNS_NUM);
		for (i = 0; i < MAX_DNS_NUM; i++) {
			system_info->network_config.dns.manul_dns[i].s_addr = 0;
		}
	} else {
		system_info->network_config.dns.dhcp_enable = 0;
		rsOnvifNetGetDNS(system_info->network_config.dns.manul_dns, MAX_DNS_NUM);
		for (i = 0; i < MAX_DNS_NUM; i++) {
			system_info->network_config.dns.dhcp_dns[i].s_addr = 0;
		}
	}

	return ret;
}

int rsOnvifSysInfoCreate(void)
{
	int shmfd;
	int ret = 0;
	RS_LOCAL_SYSTEM_INFO *info;
	mode_t old_mode;
	/*remove the existed shared memory*/
	ret = shm_unlink(SYSTEM_SHARED_MEMORY_NAME);
	if (!ret) {
		RS_DBG("failed to unlink %s errno %d\n",
			SYSTEM_SHARED_MEMORY_NAME, errno);
	}

	old_mode = umask(0);

	shmfd = shm_open(SYSTEM_SHARED_MEMORY_NAME,
			O_CREAT | O_TRUNC | O_RDWR,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (shmfd < 0) {
		RS_DBG("shm_open ret = %d\n", shmfd);
		RS_DBG("failed to shm_open() errno %d\n", errno);
		return 1;
	}
	umask(old_mode);

	ftruncate(shmfd, sizeof(RS_LOCAL_SYSTEM_INFO));

	info = (RS_LOCAL_SYSTEM_INFO *)mmap(NULL, sizeof(RS_LOCAL_SYSTEM_INFO),
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		shmfd, 0);
	if (info == NULL) {

		RS_DBG("failed to mmap\n");
		shm_unlink(SYSTEM_SHARED_MEMORY_NAME);
		return 2;
	}

	memset(info, 0, sizeof(RS_LOCAL_SYSTEM_INFO));
	/*
	old_mode = umask(0);
	mutex = sem_open(SYSTEM_SEMAPHORE_NAME, O_CREAT, 0666, 1);
	if (SEM_FAILED == mutex) {
		RS_DBG("unable to create semaphore\n");
		munmap(info, sizeof(RS_LOCAL_SYSTEM_INFO));
		shm_unlink(SYSTEM_SHARED_MEMORY_NAME);
		return 3;
	}
	umask(old_mode);

	sem_wait(mutex);
	*/
	ret = rs_sysv_sem_create();
	if (ret) {
		RS_DBG("unable to create semaphore\n");
		munmap(info, sizeof(RS_LOCAL_SYSTEM_INFO));
		return 3;
	}

	ret = rs_sysv_sem_lock();
	if (ret) {
		RS_DBG("Semaphore failed to lock\n");
		munmap(info, sizeof(RS_LOCAL_SYSTEM_INFO));
		return 3;
	}

	RS_DBG("sizeof(RS_LOCAL_SYSTEM_INFO) %d \n",
		sizeof(RS_LOCAL_SYSTEM_INFO));
	if (rsOnvifSysFillSettingsFromMemory(info)) {
		/*if failed to read setings from flash*/
		rsFillDefaultSetting(info);
	}
	/*
	sem_post(mutex);
	sem_close(mutex);
	*/
	ret = rs_sysv_sem_unlock();
	if (ret) {
		RS_DBG("Semaphore failed to unlock\n");
		return ret;
	}

	munmap((void *)info, sizeof(RS_LOCAL_SYSTEM_INFO));
	close(shmfd);
	return 0;
}

int rsOnvifSysInfoClose(void)
{
	shm_unlink(SYSTEM_SHARED_MEMORY_NAME);
	/*
	sem_unlink(SYSTEM_SEMAPHORE_NAME);
	*/
	return 0;
}

int rsOnvifSysInfoWrite2Memory(void)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}
	/*write 2 json file*/
	rsOnvifSysInfoWrite2JsonFile(local_info, SYSTEM_INFO_NAME);
	rsSystemProcessUnlock(local_info, shmfd);
	return 0;

}

int rsOnvifSysInfoGetUUID(char *uuid)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	strncpy(uuid, local_info->system_info.uuid, UUID_BUF_SIZE);

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;

}

int rsOnvifSysInfoSetUUID(char *uuid)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	strncpy(local_info->system_info.uuid, uuid, UUID_BUF_SIZE);

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetSpecialInfo(onvif_special_info_t *info)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}
	memcpy(info,
		&local_info->system_info.onvif_special_info,
		sizeof(onvif_special_info_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;

}


int rsOnvifSysInfoSetSpecialInfo(onvif_special_info_t *info)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.onvif_special_info,
		info, sizeof(onvif_special_info_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetDevInfo(device_information_t *info)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(info,
		&local_info->system_info.device_info,
		sizeof(device_information_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoSetDevInfo(device_information_t *info)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.device_info,
		info, sizeof(device_information_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetDateTimeSettings(date_time_settings_t *set)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(set,
		&local_info->system_info.date_time_settings,
		sizeof(date_time_settings_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetDateTimeSettings(date_time_settings_t *set)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.date_time_settings,
		set, sizeof(date_time_settings_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoGetNetConfig(network_config_t *config)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(config,
		&local_info->system_info.network_config,
		sizeof(network_config_t));
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoSetNetConfig(network_config_t *config)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.network_config,
		config, sizeof(network_config_t));

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetDefault(void)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}
	remove(SYSTEM_INFO_NAME);

	rsFillDefaultSetting(local_info);

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetSendHelloState(int state)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	local_info->state = state;

	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetSendHelloState(int *state)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}
	*state = local_info->state;
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetMediaProfiles(media_profile_t *profiles)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(profiles, &local_info->system_info.media_profile[0], sizeof(media_profile_t)*MAX_MEDIA_PROFILE);
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetMediaProfiles(media_profile_t *profiles)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.media_profile[0], profiles, sizeof(media_profile_t)*MAX_MEDIA_PROFILE);
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;


}

int rsOnvifSysInfoGetMediaConfigs(media_config_t *configs)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(configs, &local_info->system_info.media_config, sizeof(media_config_t));
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetMediaConfigs(media_config_t *configs)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.media_config, configs, sizeof(media_config_t));
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

int rsOnvifSysInfoGetOSDConfigs(osd_configuration_t *osd)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(osd, &local_info->system_info.osd_config[0], sizeof(osd_configuration_t)*MAX_OSD_NUM);
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}


int rsOnvifSysInfoSetOSDConfigs(osd_configuration_t *osd)
{
	RS_LOCAL_SYSTEM_INFO *local_info;
	int shmfd;
	int ret = 0;
	ret = rsSystemProcessLock((void **)&local_info, &shmfd);
	if (ret) {
		return ret;
	}

	memcpy(&local_info->system_info.osd_config[0], osd, sizeof(osd_configuration_t)*MAX_OSD_NUM);
	rsSystemProcessUnlock(local_info, shmfd);
	return ret;
}

#define MAX_PROCESS_ARG 20
int spawn_new_process(char *cmd)
{
	int ret = 0;
	pid_t child_pid;
	char *program_name;
	char *arg_list[MAX_PROCESS_ARG] = {NULL};
	char *temp;
	int arg_cnt = 0;

	if (!cmd) {
		return ret;
	}

	program_name = cmd;
	arg_list[arg_cnt++] = program_name;

	temp = strchr(cmd, ' ');
	while (temp && arg_cnt < (MAX_PROCESS_ARG - 1)) {
		*temp = 0;
		temp++;
		arg_list[arg_cnt++] = temp;
		temp = strchr(temp, ' ');
	}

	child_pid = fork();
	if (child_pid != 0) {
		return (int)child_pid;/* This is the parent process. */
	} else {
		/* Now execute PROGRAM, searching for it in the path. */
		execvp (program_name, arg_list);
		/* The execvp function returns only if an error occurs. */
		RS_DBG("an error occurred in execvp\n");
		abort ();
	}

	return ret;

}

in_addr_t rsOnvifNetGetIFAddr(char *ifname)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));

	return wan_info.ipaddr.s_addr;
}

in_addr_t rsOnvifNetGetNetmask(char *ifname)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));

	return wan_info.netmask.s_addr;
}

int rsOnvifNetGetHWAddr(char *ifname, char *mac_addr)
{
	int ret = 0;
	int msg_type = MSG_ETH_MAC;

	ret = rts_nm_get_info(msg_type, mac_addr, MAC_LENGTH);

	return ret;
}

in_addr_t rsOnvifNetGetGateway(void)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));

	return wan_info.gateway.s_addr;
}


int rsOnvifNetSetGateway(in_addr_t addr)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		goto out;

	wan_info.gateway.s_addr = addr;
	ret = rts_nm_set_info(msg_type, &wan_info, sizeof(struct nm_wan));
out:
	return ret;
}

int rsOnvifNetSetDNS(struct in_addr addr[], int num)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		goto out;

	if (num == 1) {
		wan_info.dns1.s_addr = addr[0].s_addr;
		ret = rts_nm_set_info(msg_type, &wan_info, sizeof(struct nm_wan));
		if (ret < 0)
			goto out;
	} else if (num > 1) {
		wan_info.dns1.s_addr = addr[0].s_addr;
		wan_info.dns2.s_addr = addr[1].s_addr;
		ret = rts_nm_set_info(msg_type, &wan_info, sizeof(struct nm_wan));
		if (ret < 0)
			goto out;
	}
out:
	return ret;
}

int rsOnvifNetGetDNS(struct in_addr addr[], int num)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		goto out;

	if (num == 1) {
		addr[0].s_addr = wan_info.dns1.s_addr;
	} else if (num > 1) {
		addr[0].s_addr = wan_info.dns1.s_addr;
		addr[1].s_addr = wan_info.dns2.s_addr;
	}
out:
	return ret;
}


int rsOnvifNetSetSearchDomain(char **domain, int num)
{
	int ret = 0;
	/*wait hurry_niu to implement*/
#if 0
	FILE *fp;
	int ret = 0;
	char *buffer;
	int size = MAX_LINE_NUMBER_RESOLV_CONF*MAX_LINE_LENGTH_RESOLV_CONF;
	int counter = 0;

	int step = 0;

	buffer = (char *)malloc(size);
	if (buffer) {
		memset(buffer, 0, size);
		fp = fopen(RESOLV_CONF, "r");
		if (fp) {
			char *pbuf = buffer;
			while (!feof(fp) && counter < MAX_LINE_NUMBER_RESOLV_CONF) {
				fgets(pbuf, MAX_LINE_LENGTH_RESOLV_CONF, fp);
				/*search domain;*/
				if (!strncmp(pbuf, "search", 6)) {
					int i;
					memset(pbuf, 0, MAX_LINE_LENGTH_RESOLV_CONF);
					sprintf(pbuf, "search");
					for (i = 0; i < num; i++) {
						if ((strlen(pbuf) + strlen(domain[i]) + 2) >= 256)
							break;
						sprintf(pbuf + strlen(pbuf), " %s", domain[i]);
					}
					sprintf(pbuf + strlen(pbuf), "\n");
					step = 1;
				}
				counter++;
				pbuf += MAX_LINE_LENGTH_RESOLV_CONF;

			}
			fclose(fp);
		} else {
			RS_DBG("failed to open file %s, Line %d\n", RESOLV_CONF, __LINE__);
			ret = -1;
		}
		RS_DBG("COUNTER %d\n", counter);
		fp = fopen(RESOLV_CONF, "w");
		if (fp) {
			int i;
			char *pbuf = buffer;
			/*insert search line at the begin;*/
			if (step == 0) {
				char *search_buffer = (char *)malloc(260);
				memset(search_buffer, 0, 260);
				sprintf(search_buffer, "search");
				for (i = 0; i < num; i++) {
					if ((strlen(search_buffer) + strlen(domain[i]) + 2) >= 256)
						break;
					sprintf(search_buffer + strlen(search_buffer), " %s", domain[i]);
				}
				sprintf(search_buffer + strlen(search_buffer), "\n");
				fputs(search_buffer, fp);
				free(search_buffer);


			}
			for (i = 0; i < counter; i++) {
				fputs(pbuf, fp);
				pbuf += MAX_LINE_LENGTH_RESOLV_CONF;
			}
			fclose(fp);
		} else {
			RS_DBG("failed to open file %s, Line %d\n", RESOLV_CONF, __LINE__);
			ret = -1;
		}
		free(buffer);
	} else {

		RS_DBG("failed to malloc\n");
		ret = -1;
	}
#endif
	return ret;
}

/* for Fill Default Setting */
int rsOnvifNetGetNetworkInterfaces(char name[][20], int *interface_num)
{
	RS_DBG("In func:%s, line:%d\n", __func__, __LINE__);
	uint8 i = 0;
	*interface_num = 1;
	strcpy(name[0], SYSTEM_IFNAME);

	*interface_num = CUR_USABLE_INTERFACE_NUM;
	/*
	for (i=0; i< CUR_USABLE_INTERFACE_NUM; i++)
		sprintf(name[i],"eth%d",i);
	*/

	return 0;

}

/*
* Set IP Address
*/
int rsOnvifNetSetStaticIPAddr(char *name,
		struct in_addr ip_addr,
		struct in_addr netmask,
		struct in_addr gateway)
{
	struct nm_wan wan_info;
	int ret = 0;

	ret = rts_nm_get_info(MSG_WAN_INFO, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		goto out;

	wan_info.dhcpc = 0;
	wan_info.ipaddr.s_addr = ip_addr.s_addr;
	wan_info.netmask.s_addr = netmask.s_addr;
	wan_info.gateway.s_addr = gateway.s_addr;

	ret = rts_nm_set_info(MSG_WAN_INFO, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		goto out;

out:
	return ret;
}

int rsOnvifNetEnableDHCP(char *name)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		return ret;

	if (wan_info.dhcpc != DHCP_MODE) {
		wan_info.dhcpc = DHCP_MODE;
		ret = rts_nm_set_info(msg_type, &wan_info, sizeof(struct nm_wan));
		if (ret < 0)
			return ret;
	}

	return ret;
}


int rsOnvifNetDisableDHCP(char *name)
{
	struct nm_wan wan_info;
	int msg_type = MSG_WAN_INFO;
	int ret = 0;

	ret = rts_nm_get_info(msg_type, &wan_info, sizeof(struct nm_wan));
	if (ret < 0)
		return ret;

	if (wan_info.dhcpc != STATIC_MODE) {
		wan_info.dhcpc = STATIC_MODE;
		ret = rts_nm_set_info(msg_type, &wan_info, sizeof(struct nm_wan));
		if (ret < 0)
			return ret;
	}

	return ret;
}

int rsOnvifNetConfigProtocol(char *name, int enabled, unsigned short port[6])
{
	int ret = 0;
	/*need to do*/
	return ret;
}

void rsOnvifSysGetDevUUID(char *uuid, int length)
{
	int ret;

	int msg_type = MSG_ETH_MAC;
	unsigned char mac_addr[MAC_LENGTH] = {0};
	ret = rts_nm_get_info(msg_type, mac_addr, MAC_LENGTH);
	if (ret < 0) {
		RS_DBG("Failed to get MAC ADDR");
	}

	sprintf(uuid,
		"20140201-2ed2-d7e4-10bf-%02X%02X%02X%02X%02X%02X",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);
}

void rsOnvifSysGetDevSerialNumber(char *sn, int length)
{
	sprintf(sn, "10000000");
}

int rsOnvifSysChangeDstTimeZone(int dst, char *tz)
{
	char tzcmd[100];
	RS_DBG("%s: dst %d, tz %s\n", __func__, dst, tz);
	/*
	daylight = dst;
	setenv("TZ",tz, 1);
	tzset();
	RS_DBG("changeTimeZone:\n %s\n", tzcmd);
	*/
	int ret = 0;

	char command[100], str_cmd[128] = {0};
	unsigned char *str_tz1;

	if (!strcmp(tz, "NT")) {
		sprintf(command, "GMT11");
	} else if (!strcmp(tz, "AHST")) {
		sprintf(command, "GMT10");
	} else if (!strcmp(tz, "AKST")) {
		if (dst)
			sprintf(command, "GMT9%s",
				"PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
		else
			sprintf(command, "GMT9");
	} else if (!strcmp(tz, "PST")) {
		if (dst)
			sprintf(command, "GMT8%s",
				"PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
		else
			sprintf(command, "GMT8");
		/*GMT-08:00)*/
	} else if (!strcmp(tz, "MST")) {
		if (dst)
			sprintf(command, "GMT7%s",
				"PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
		else
			sprintf(command, "GMT7");
		/*GMT-07:00) 7 2 */
	} else if (!strcmp(tz, "CST")) {
		if (dst)
			sprintf(command, "GMT6%s",
				"PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
		else
			sprintf(command, "GMT6");
		/*GMT-06:00) 6 1 */
	} else if (!strcmp(tz, "EST")) {
		if (dst)
			sprintf(command, "GMT5%s",
				"PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
			else
				sprintf(command, "GMT5");
			/*GMT-05:00) 5 2 */
	} else if (!strcmp(tz, "AST")) {
		sprintf(command, "GMT4");
		/*GMT-04:00) 4 1*/
	} else if (!strcmp(tz, "BRT")) {
		if (dst)
			sprintf(command, "GMT3%s",
				"PDT,M2.2.0/00:00:00,M10.2.0/00:00:00");
		else
			sprintf(command, "GMT3");
		/*GMT-03:00) 3 2*/
	} else if (!strcmp(tz, "NDT")) {
		sprintf(command, "GMT2");
		/*GMT-02:00)*/
	} else if (!strcmp(tz, "WAT")) {
		if (dst)
			sprintf(command, "GMT1%s",
				"PDT,M3.5.0/00:00:00,M10.5.0/01:00:00");
		else
			sprintf(command, "GMT1");
		/*GMT-01:00) 1 1*/
	} else if (!strcmp(tz, "UTC")) {
		if (dst)
			sprintf(command, "GMT0%s",
				"PDT,M3.5.0/01:00:00,M10.5.0/02:00:00");
		else
			sprintf(command, "GMT0");
		/*GMT) 0 2*/
	} else if (!strcmp(tz, "CET")) {
		if (dst)
			sprintf(command, "GMT-1%s",
				"PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
		else
			sprintf(command, "GMT-1");
		/*GMT+01:00)*/
	} else if (!strcmp(tz, "SST")) {
		sprintf(command, "GMT-2");
		/*GMT+02:00)*/
	} else if (!strcmp(tz, "HMT")) {
		sprintf(command, "GMT-3");
		/*GMT+03:00*/
	} else if (!strcmp(tz, "MUT")) {
		sprintf(command, "GMT-4");
		/*GMT+04:00*/
	} else if (!strcmp(tz, "MVT")) {
		sprintf(command, "GMT-5");
		/*GMT+05:00)*/
	} else if (!strcmp(tz, "ALMT")) {
		sprintf(command, "GMT-6");
		/*GMT+06:00)*/
	} else if (!strcmp(tz, "CXT")) {
		sprintf(command, "GMT-7");
		/*GMT+07:00)*/
	} else if (!strcmp(tz, "CCT")) {
		sprintf(command, "GMT-8");
		/*GMT+08:00)*/
	} else if (!strcmp(tz, "JST")) {
		sprintf(command, "GMT-9");
		/*GMT+09:00)*/
	} else if (!strcmp(tz, "LIGT")) {
		if (dst)
			sprintf(command, "GMT-10%s",
				"PDT,M10.5.0/02:00:00,M4.1.0/03:00:00");
		else
			sprintf(command, "GMT-10");
		/*GMT+10:00) -10 2*/
	} else if (!strcmp(tz, "AESST")) {
		sprintf(command, "GMT-11");
		/*GMT+11:00)*/
	} else if (!strcmp(tz, "NZT")) {
		sprintf(command, "GMT-12");
		/*GMT+12:00)*/
	}
#define TZ_FILE "/var/conf/TZ"
	unlink(TZ_FILE);
	sprintf(str_cmd, "echo %s >%s", command, TZ_FILE);
	system(str_cmd);
#undef TZ_FILE
	return 0;
}


int rsOnvifUsrAuthentication(struct soap *soap, enum tt__UserLevel user_level)
{
	struct _wsse__Security *sec;
	struct _wsse__UsernameToken *token;
	/*the local password of username  matched with the sender;*/
	char *pwd;
	/*the local user level of username matched with the sender;*/
	enum tt__UserLevel level = tt__UserLevel__Extended;
	int user_idx;
	int ret;
	network_config_t *network;

	if (!soap->header) {
		return 0;
	}

	sec = soap->header->wsse__Security;
	/*if there is no authentication information, then return false;*/
	if (!sec) {
		return 0;
	}
	token = sec->UsernameToken;
	/*there is authentication infomation without username_token, return false;*/
	if (!token) {
		return 0;
	}
	/*if there is something lost, return false;*/
	if (!token->Username ||
		!token->Nonce ||
		!token->Password ||
		!token->Password->Type ||
		!token->wsu__Created) {
	   return 0;
	}

	/*check if the username is valid;*/
	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		return 0;

	user_idx = 0;
	pwd = NULL;
	ASSERT(strlen(token->Username) < USER_LEN);
	while (user_idx < MAX_ACCOUNT_NUM) {
		if (!strncmp(network->accounts[user_idx].user, token->Username, strlen(token->Username))) {
			pwd = network->accounts[user_idx].password;
			level = (enum tt__UserLevel)network->accounts[user_idx].authority;
			break;
		}
		user_idx++;
	}

	/*if there is no user matched;*/
	if (pwd == NULL) {
		return 0;
	}
	/*if it has the lower priority, return false;*/
	if (level > user_level) {
		return 0;
	}

	LIBAUTH_VERIFY_ONVIF auth;
	memset(&auth, 0, sizeof(LIBAUTH_VERIFY_ONVIF));
	strncpy(auth.user_name, token->Username, strlen(token->Username));
	strncpy(auth.nonce, token->Nonce, strlen(token->Nonce));
	strncpy(auth.date, token->wsu__Created, strlen(token->wsu__Created));
	strncpy(auth.verify_onvif, token->Password->__item,
			strlen(token->Password->__item));

	RS_DBG("username input  :%s\n", token->Username);
	RS_DBG("nonce    input  :%s\n", token->Nonce);
	RS_DBG("date     input  :%s\n", token->wsu__Created);
	RS_DBG("digest   input  :%s\n", token->Password->__item);

	RS_DBG("username:%s\n", auth.user_name);
	RS_DBG("nonce   :%s\n", auth.nonce);
	RS_DBG("date    :%s\n", auth.date);
	RS_DBG("pwddigst:%s\n", auth.verify_onvif);

	rts_auth_init();
	if (!rts_auth_check_onvif(&auth))
		ret = 1;
	else
		ret = 0;
	RS_DBG("lib auth ret = %d\n", ret);
	rts_auth_uninit();

	return ret;
}
