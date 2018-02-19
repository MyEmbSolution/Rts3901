#ifndef __LIBAUTH_H__
#define __LIBAUTH_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define AUTH_MAX_PATH 2048

/* for add and remove user */
typedef struct _AUTH_ADD_REMOVE_USER {
	char user_name[AUTH_MAX_PATH];
	char pass_word[AUTH_MAX_PATH];
} LIBAUTH_ADD_REMOVE_USER;

/* for modify user password */
typedef struct _AUTH_CHANGE_PASSWD {
	char user_name[AUTH_MAX_PATH];
	char ori_pass_wd[AUTH_MAX_PATH];
	char new_pass_wd[AUTH_MAX_PATH];
} LIBAUTH_CHANGE_PASSWD;

/* for verify with raw */
typedef struct _AUTH_VERIFY_RAW {
	char user_name[AUTH_MAX_PATH];
	char verify_raw[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_RAW;

/* for verify with MD5 */
typedef struct _AUTH_VERIFY_MD5 {
	char user_name[AUTH_MAX_PATH];
	char verify_md5[16];
} LIBAUTH_VERIFY_MD5;

/* for verify with BASE64 */
typedef struct _AUTH_VERIFY_BASE64 {
	char user_name[AUTH_MAX_PATH];
	char verify_base64[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_BASE64;

/* for verify under OnVif */
typedef struct _AUTH_VERIFY_ONVIF {
	char user_name[AUTH_MAX_PATH];
	char nonce[AUTH_MAX_PATH];
	char date[AUTH_MAX_PATH];
	char verify_onvif[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_ONVIF;

/* for verify with MD5 */
typedef struct _AUTH_VERIFY_RSA {
	char user_name[AUTH_MAX_PATH];
	char verify_rsa[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_RSA;

/* for verify under RTSP */
typedef struct _AUTH_VERIFY_RTSP {
	char user_name[AUTH_MAX_PATH];
	char realm[AUTH_MAX_PATH];
	char nonce[AUTH_MAX_PATH];
	char cmd[AUTH_MAX_PATH];
	char url[AUTH_MAX_PATH];
	char verify_rtsp[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_RTSP;

/* for verify under lighttpd */
typedef struct _AUTH_VERIFY_LIGHTTPD {
	char user_name[AUTH_MAX_PATH];
	char realm[AUTH_MAX_PATH];
	char algorithm[AUTH_MAX_PATH];
	char nonce[AUTH_MAX_PATH];
	char cnonce[AUTH_MAX_PATH];
	char method[AUTH_MAX_PATH];
	char uri[AUTH_MAX_PATH];
	char nc[AUTH_MAX_PATH];
	char qop[AUTH_MAX_PATH];
	char verify_lighttpd[AUTH_MAX_PATH];
} LIBAUTH_VERIFY_LIGHTTPD;

/* lib init */
int rts_auth_init();
/* add user */
int rts_auth_add_user(LIBAUTH_ADD_REMOVE_USER *param);
/* remove user */
int rts_auth_remove_user(LIBAUTH_ADD_REMOVE_USER *param);
/* modify pass word */
int rts_auth_modify_password(LIBAUTH_CHANGE_PASSWD *param);
/* auth check raw */
int rts_auth_check_raw(LIBAUTH_VERIFY_RAW *param);
/* auth check md5 */
int rts_auth_check_md5(LIBAUTH_VERIFY_MD5 *param);
/* auth check base64 */
int rts_auth_check_base64(LIBAUTH_VERIFY_BASE64 *param);
/* auth check rsa */
int rts_auth_check_rsa(LIBAUTH_VERIFY_RSA *param);
/* auth check onvif */
int rts_auth_check_onvif(LIBAUTH_VERIFY_ONVIF *param);
/* auth check rtsp */
int rts_auth_check_rtsp(LIBAUTH_VERIFY_RTSP *param);
/* auth check lighttpd */
int rts_auth_check_lightpd(LIBAUTH_VERIFY_LIGHTTPD *param);
/* lib unint */
int rts_auth_uninit();

int rts_md5_encrypt(unsigned char *data, int data_len, unsigned char *encrypted);
bool rts_base64_decode(const char *in, size_t inlen, char *out, size_t *outlen);
void rts_base64_encode (const char *in, size_t inlen, char *out, size_t outlen);

#endif
