#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <mntent.h>
#include "ft2log.h"
#include "ft2errno.h"
#include "ft2_test_eth.h"

#include "tester.h"

int set_ip_address(char *ipaddr)
{
	char cmd[32] = {0};
	char buf[4] = {0};
	int ret = FT2_OK;
	unsigned int i;
	FILE *fp;

	if (NULL == ipaddr)
		return -FT2_NULL_POINT;

	/* enable eth0 up */
	snprintf(cmd, 32, "ifconfig eth0 %s up", ipaddr);
	if (-1 == system(cmd))
		ret = -FT2_ERROR;

	/* read eth0 status */
	fp = fopen(_PATH_ETH0_OPERSTATE, "r");
	if (!fp) {
		FT2_LOG_ERR("can't open %s\n", _PATH_ETH0_OPERSTATE);
		return -FT2_ERROR;
	}

	sleep(5);
	for (i = 0; i < 20; i++) {
		memset(buf, 0, sizeof(buf));
		fseek(fp, 0, SEEK_SET);
		if(fgets(buf, sizeof(buf), fp) != NULL) {
			if (!strncmp(buf, "up", 2)) {
				ret = FT2_OK;
				break;
			}
		}
		sleep(1);
	}

	if (i == 20) {
		FT2_LOG_ERR("read %s time out\n", _PATH_ETH0_OPERSTATE);
		ret = -FT2_ERROR;
	}

	fclose(fp);
	return ret;
}

int check_eth_speed()
{
	char buf[6] = {'#', '#', '#', '#', '#', '#'};
	int speed = MODE_UNKNOWN;
	char *ret = NULL;
	FILE *fp;

	fp = fopen(_PATH_SYSNET_SPEED, "r");
	if (!fp)
		return -FT2_ERROR;

	ret = fgets(buf, sizeof(buf), fp);
	if (ret) {
		if (!strcmp(buf, "100\n"))
			speed = MODE_100;
		else if (!strcmp(buf, "10\n"))
			speed = MODE_10;
		else
			speed = MODE_UNKNOWN;
	}

	fclose(fp);

	if (MODE_100 == speed)
		return FT2_OK;
	else
		return -FT2_ERROR;
}

int mnt_is_mounted()
{
	char *filename = "/proc/mounts";
	FILE *mntfile;
	struct mntent *mntent;
	int ret = 0;

	mntfile = setmntent(filename, "r");
	if (!mntfile) {
		FT2_LOG_INFO("Failed to read /proc/mounts\n");
		return -FT2_ERROR;
	}

	while(mntent = getmntent(mntfile))
		if (!strncmp("/mnt", mntent->mnt_dir, 4))
			ret = 1;

	endmntent(mntfile);

	return ret;
}

int mount_nfs(char *nfs_addr)
{
	char cmd[52] = {0};

	if (NULL == nfs_addr)
                return -FT2_NULL_POINT;

	if (mnt_is_mounted()) {
		FT2_LOG_INFO("mnt have been mounted before\n");
		return FT2_OK;
	}

	snprintf(cmd, 52, "mount -t nfs -o nolock %s:/IpcamTest /mnt", nfs_addr);
        if (-1 == system(cmd))
		return -FT2_ERROR;

	usleep(10000);

	if (mnt_is_mounted()) {
		FT2_LOG_INFO("mnt mount ok\n");
		return FT2_OK;
	} else {
		return -FT2_ERROR;
	}
}

int mount_cifs(char *cifs_addr, char *mnt_filename)
{
        char cmd[84] = {0};

        if (NULL == cifs_addr)
                return -FT2_NULL_POINT;

        if (mnt_is_mounted()) {
                FT2_LOG_INFO("mnt have been mounted before\n");
                return FT2_OK;
        }

	snprintf(cmd, 84,
		"mount -t cifs -o username=rsipcam,password=rsipcam //%s/%s /mnt",
		cifs_addr, mnt_filename);
        if (-1 == system(cmd))
                return -FT2_ERROR;

        usleep(10000);

        if (mnt_is_mounted()) {
                FT2_LOG_INFO("mnt mount ok\n");
                return FT2_OK;
        } else {
                return -FT2_ERROR;
        }
}

int net_init(struct ft2_opt_params *params)
{
	int ret = 0;

	ret = set_ip_address(params->ipcam_addr);
	if (ret) {
		FT2_LOG_ERR("set ip address fail\n");
		return ret;
	}

	ret = check_eth_speed();
	if (ret) {
		FT2_LOG_ERR("eth speed unusual\n");
		return ret;
	}

	if (!strcmp(params->fs_type, "cifs"))
		ret = mount_cifs(params->host_addr, params->mnt_filename);
	else if (!strcmp(params->fs_type, "nfs"))
		ret = mount_nfs(params->host_addr);
	else
		ret = -1;
	if (ret) {
		FT2_LOG_ERR("mount mnt failed\n");
		return ret;
	}
}

void net_exit()
{
	system("umount /mnt");
}
