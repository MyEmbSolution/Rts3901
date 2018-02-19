#ifndef __AUTH_FUNC_H__
#define __AUTH_FUNC_H__
#include <string.h>
#include "auth.h"
#include <openssl/rsa.h>
#include <openssl/md5.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "list.h"

#define AUTH_MAX_PATH 2048

typedef enum {
	AUTH_TYPE_RAW = 0,
	AUTH_TYPE_MD5,
	AUTH_TYPE_BASE64,
	AUTH_TYPE_ONVIF,
	AUTH_TYPE_RSA,
	AUTH_TYPE_RTSP,
	AUTH_TYPE_LIGHTTPD,
	AUTH_TYPE_MAX
} AUTH_TYPE;

typedef struct _AUTH_PARAM {

	struct list_head link;
	AUTH_TYPE type;

	union {
		/* for add and remove user */
		LIBAUTH_ADD_REMOVE_USER AUTH_ADD_REMOVE_USER;

		/* for modify user password */
		LIBAUTH_CHANGE_PASSWD AUTH_CHANGE_PASSWD;

		/* for verify with raw */
		LIBAUTH_VERIFY_RAW AUTH_VERIFY_RAW;

		/* for verify with MD5 */
		LIBAUTH_VERIFY_MD5 AUTH_VERIFY_MD5;

		/* for verify with BASE64 */
		LIBAUTH_VERIFY_BASE64 AUTH_VERIFY_BASE64;

		/* for verify under OnVif */
		LIBAUTH_VERIFY_ONVIF AUTH_VERIFY_ONVIF;

		/* for verify with MD5 */
		LIBAUTH_VERIFY_RSA AUTH_VERIFY_RSA;

		/* for verify under RTSP */
		LIBAUTH_VERIFY_RTSP AUTH_VERIFY_RTSP;

		/* for verify under lighttpd */
		LIBAUTH_VERIFY_LIGHTTPD AUTH_VERIFY_LIGHTTPD;
	};
} AUTH_PARAM;

/* add user */
int rts_auth_add_user_comm(AUTH_PARAM *auth_param);
/* remove user */
int rts_auth_remove_user_comm(AUTH_PARAM *auth_param);
/* modify pass word */
int rts_auth_modify_password_comm(AUTH_PARAM *auth_param);
/* auth check */
int rts_auth_check_comm(AUTH_PARAM *auth_param);

/* log utils */
enum {
	LIBAUTH_LOG_FINE		= (1 << 0),
	LIBAUTH_LOG_INFO		= (1 << 1),
	LIBAUTH_LOG_WARNING		= (1 << 2),
	LIBAUTH_LOG_ERROR		= (1 << 3),
	LIBAUTH_LOG_FATAL		= (1 << 4),
};

extern uint32_t libauth_log_level;

#ifndef au_log
#define au_log(level, fmt, args...)		\
	do { \
		if (level & libauth_log_level) {	\
			syslog(LOG_ERR, "%-20s--> "fmt, __func__, ##args); \
		} \
	} while (0)

#define au_log_fine(fmt, args...) au_log(LIBAUTH_LOG_FINE, fmt, ##args)
#define au_log_info(fmt, args...) au_log(LIBAUTH_LOG_INFO, fmt, ##args)
#define au_log_warning(fmt, args...) au_log(LIBAUTH_LOG_WARNING, fmt, ##args)
#define au_log_error(fmt, args...) au_log(LIBAUTH_LOG_ERROR, fmt, ##args)
#define au_log_fatal(fmt, args...) au_log(LIBAUTH_LOG_FATAL, fmt, ##args)
#endif

#endif
