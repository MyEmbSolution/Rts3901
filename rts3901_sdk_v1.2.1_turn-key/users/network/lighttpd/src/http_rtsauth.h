#ifndef _HTTP_RTSAUTH_H_
#define _HTTP_RTSAUTH_H_

#include "server.h"
#include "plugin.h"

#if defined(HAVE_LDAP_H) && defined(HAVE_LBER_H) && defined(HAVE_LIBLDAP) && defined(HAVE_LIBLBER)
# define USE_LDAP
# include <ldap.h>
#endif

typedef enum {
	RTSAUTH_BACKEND_UNSET,
	RTSAUTH_BACKEND_PLAIN,
	RTSAUTH_BACKEND_LDAP,
	RTSAUTH_BACKEND_HTPASSWD,
	RTSAUTH_BACKEND_HTDIGEST
} rtsauth_backend_t;

typedef struct {
	/* rtsauth */
	array  *rtsauth_require;

	buffer *rtsauth_plain_groupfile;
	buffer *rtsauth_plain_userfile;

	buffer *rtsauth_htdigest_userfile;
	buffer *rtsauth_htpasswd_userfile;

	buffer *rtsauth_backend_conf;

	buffer *rtsauth_ldap_hostname;
	buffer *rtsauth_ldap_basedn;
	buffer *rtsauth_ldap_binddn;
	buffer *rtsauth_ldap_bindpw;
	buffer *rtsauth_ldap_filter;
	buffer *rtsauth_ldap_cafile;
	unsigned short rtsauth_ldap_starttls;
	unsigned short rtsauth_ldap_allow_empty_pw;

	unsigned short rtsauth_debug;

	/* generated */
	rtsauth_backend_t rtsauth_backend;

#ifdef USE_LDAP
	LDAP *ldap;

	buffer *ldap_filter_pre;
	buffer *ldap_filter_post;
#endif
} mod_rtsauth_plugin_config;

typedef struct {
	PLUGIN_DATA;
	buffer *tmp_buf;

	buffer *rtsauth_user;

#ifdef USE_LDAP
	buffer *ldap_filter;
#endif

	mod_rtsauth_plugin_config **config_storage;

	mod_rtsauth_plugin_config conf, *anon_conf; /* this is only used as long as no handler_ctx is setup */
} mod_rtsauth_plugin_data;

int http_rtsauth_basic_check(server *srv, connection *con, mod_rtsauth_plugin_data *p, array *req, const char *realm_str);
int http_rtsauth_digest_check(server *srv, connection *con, mod_rtsauth_plugin_data *p, array *req, const char *realm_str);
int http_rtsauth_digest_generate_nonce(server *srv, mod_rtsauth_plugin_data *p, buffer *fn, char hh[33]);
int http_rtsauth_match_rules(server *srv, array *req, const char *username, const char *group, const char *host);

#endif
