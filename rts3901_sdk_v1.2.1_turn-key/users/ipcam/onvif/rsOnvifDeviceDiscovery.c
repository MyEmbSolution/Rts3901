#include <signal.h>
#include <syslog.h>

#include "soapH.h"
#include "rsOnvifDefines.h"
#include "rsOnvifSystemCtrl.h"


/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

static int daemonize()
{
	int fd;
	/*
	 * become a session leader to lose controlling tty
	 */
	switch (fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		return -2;

	switch (fork()) {
	case -1:
		return -3;
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	/*
	 * clear file creation mask
	 */
	umask(0);
	/*
	 * change the current working directory to the root so we won't prevent
	 * file systems from being umounted.
	 */
	chdir("/");

	/*
	 * close open file descriptors
	 */
	for (fd = 0; fd < 3; fd++)
		close(fd);

	fd = open("/dev/null", O_RDWR);

	if (fd != STDIN_FILENO)
		return -4;
	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
		return -5;
	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
		return -6;

	return 0;
}

int main(int argc, void *argv[])
{
	int ret = 0;
	pthread_t hello_thread_handle;
	pthread_t monitor_thread_handle;
	pthread_t devCtrl_thread_handle;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL3);

	ret = daemonize();
	if (ret) {
		RS_DBG("daemonize failed with ret = %d", ret);
		return -1;
	}

	RS_DBG("%s start!\n", (char *)argv[0]);

	/*if the network device is not ready;*/
	/*	while (rsOnvifNetGetIFAddr("eth0") == -1)*/
	while (rsOnvifNetGetIFAddr(SYSTEM_IFNAME) == -1) {
		RS_DBG("network is not ready!\n");
		sleep(4);
	}

	ret = rsOnvifSysInfoCreate();
	RS_DBG("crate System info ret %d\n", ret);
	if (ret)
		goto end;

	/*create 3 threads*/
	/*send hello msg*/
	pthread_create(&hello_thread_handle, NULL,
			rsOnvifWSDDHelloThread, NULL);

	/*listen probe&resolve msg*/
	pthread_create(&monitor_thread_handle, NULL,
			rsOnvifWSDDMonitorMsgThread, NULL);

	/*control device thread*/
	pthread_create(&devCtrl_thread_handle, NULL,
			rsOnvifSysDevCtrlThread, NULL);

	signal(SIGCHLD, SIG_IGN);
	/*wait thread to exit*/
	pthread_join(hello_thread_handle, NULL);
	pthread_join(monitor_thread_handle, NULL);
	pthread_join(devCtrl_thread_handle, NULL);


	rsOnvifSysInfoClose();

end:
	return 0;

}



