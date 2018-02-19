#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <auth/auth.h>
#include "soapH.h"
#include "rsOnvifDefines.h"
#include "rsOnvifSystemCtrl.h"
#include "rsOnvifCommonFunc.h"

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

int main(int argc, void *argv[])
{
	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL3);
	RS_DBG("Start CGI\n");

	soap_serve(soap_new());

	/*
	soap_destroy(&soap);
	soap_end(&soap);
	soap_done(&soap);
	*/
	return 0;
}
