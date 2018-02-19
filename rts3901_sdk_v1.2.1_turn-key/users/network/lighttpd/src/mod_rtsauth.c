#include "plugin.h"
#include "http_rtsauth.h"
#include "log.h"
#include "response.h"
#include "mod_auth.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

handler_t rtsauth_ldap_init(server *srv, mod_rtsauth_plugin_config *s);

/**
 * the basic and digest rtsauth framework
 *
 * - config handling
 * - protocol handling
 *
 * http_rtsauth.c
 * http_rtsauth_digest.c
 *
 * do the real work
 */

INIT_FUNC(mod_rtsauth_init) {
	printf("mod_rtsauth_init\n");
	mod_rtsauth_plugin_data *p;

	p = calloc(1, sizeof(*p));

	p->tmp_buf = buffer_init();

	p->rtsauth_user = buffer_init();
#ifdef USE_LDAP
	p->ldap_filter = buffer_init();
#endif

	return p;
}

FREE_FUNC(mod_rtsauth_free) {
	printf("mod_rtsauth_free\n");
	mod_rtsauth_plugin_data *p = p_d;

	UNUSED(srv);

	if (!p) return HANDLER_GO_ON;

	buffer_free(p->tmp_buf);
	buffer_free(p->rtsauth_user);
#ifdef USE_LDAP
	buffer_free(p->ldap_filter);
#endif

	if (p->config_storage) {
		size_t i;
		for (i = 0; i < srv->config_context->used; i++) {
			mod_rtsauth_plugin_config *s = p->config_storage[i];

			if (!s) continue;

			array_free(s->rtsauth_require);
			buffer_free(s->rtsauth_plain_groupfile);
			buffer_free(s->rtsauth_plain_userfile);
			buffer_free(s->rtsauth_htdigest_userfile);
			buffer_free(s->rtsauth_htpasswd_userfile);
			buffer_free(s->rtsauth_backend_conf);

			buffer_free(s->rtsauth_ldap_hostname);
			buffer_free(s->rtsauth_ldap_basedn);
			buffer_free(s->rtsauth_ldap_binddn);
			buffer_free(s->rtsauth_ldap_bindpw);
			buffer_free(s->rtsauth_ldap_filter);
			buffer_free(s->rtsauth_ldap_cafile);

#ifdef USE_LDAP
			buffer_free(s->ldap_filter_pre);
			buffer_free(s->ldap_filter_post);

			if (s->ldap) ldap_unbind_s(s->ldap);
#endif

			free(s);
		}
		free(p->config_storage);
	}

	free(p);

	return HANDLER_GO_ON;
}

#define PATCH(x) \
	p->conf.x = s->x;
static int mod_rtsauth_patch_connection(server *srv, connection *con, mod_rtsauth_plugin_data *p) {
	size_t i, j;
	mod_rtsauth_plugin_config *s = p->config_storage[0];
	log_error_write(srv, __FILE__, __LINE__, "s", "mod_rtsauth_patch_connection");

	PATCH(rtsauth_backend);
	PATCH(rtsauth_plain_groupfile);
	PATCH(rtsauth_plain_userfile);
	PATCH(rtsauth_htdigest_userfile);
	PATCH(rtsauth_htpasswd_userfile);
	PATCH(rtsauth_require);
	PATCH(rtsauth_debug);
	PATCH(rtsauth_ldap_hostname);
	PATCH(rtsauth_ldap_basedn);
	PATCH(rtsauth_ldap_binddn);
	PATCH(rtsauth_ldap_bindpw);
	PATCH(rtsauth_ldap_filter);
	PATCH(rtsauth_ldap_cafile);
	PATCH(rtsauth_ldap_starttls);
	PATCH(rtsauth_ldap_allow_empty_pw);
#ifdef USE_LDAP
	p->anon_conf = s;
	PATCH(ldap_filter_pre);
	PATCH(ldap_filter_post);
#endif

	/* skip the first, the global context */
	for (i = 1; i < srv->config_context->used; i++) {
		data_config *dc = (data_config *)srv->config_context->data[i];
		s = p->config_storage[i];

		/* condition didn't match */
		if (!config_check_cond(srv, con, dc)) continue;

		/* merge config */
		for (j = 0; j < dc->value->used; j++) {
			data_unset *du = dc->value->data[j];

			if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend"))) {
				PATCH(rtsauth_backend);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.plain.groupfile"))) {
				PATCH(rtsauth_plain_groupfile);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.plain.userfile"))) {
				PATCH(rtsauth_plain_userfile);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.htdigest.userfile"))) {
				PATCH(rtsauth_htdigest_userfile);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.htpasswd.userfile"))) {
				PATCH(rtsauth_htpasswd_userfile);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.require"))) {
				PATCH(rtsauth_require);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.debug"))) {
				PATCH(rtsauth_debug);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.hostname"))) {
				PATCH(rtsauth_ldap_hostname);
#ifdef USE_LDAP
				p->anon_conf = s;
#endif
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.base-dn"))) {
				PATCH(rtsauth_ldap_basedn);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.filter"))) {
				PATCH(rtsauth_ldap_filter);
#ifdef USE_LDAP
				PATCH(ldap_filter_pre);
				PATCH(ldap_filter_post);
#endif
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.ca-file"))) {
				PATCH(rtsauth_ldap_cafile);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.starttls"))) {
				PATCH(rtsauth_ldap_starttls);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.bind-dn"))) {
				PATCH(rtsauth_ldap_binddn);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.bind-pw"))) {
				PATCH(rtsauth_ldap_bindpw);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("rtsauth.backend.ldap.allow-empty-pw"))) {
				PATCH(rtsauth_ldap_allow_empty_pw);
			}
		}
	}

	return 0;
}
#undef PATCH

static handler_t mod_rtsauth_uri_handler(server *srv, connection *con, void *p_d) {
	size_t k;
	int rtsauth_required = 0, rtsauth_satisfied = 0;
	char *http_rtsauthorization = NULL;
	const char *rtsauth_type = NULL;
	data_string *ds;
	mod_rtsauth_plugin_data *p = p_d;
	array *req;
	data_string *req_method;

	log_error_write(srv, __FILE__, __LINE__, "s", "mod_rtsauth_uri_handler");
	/* select the right config */
	mod_rtsauth_patch_connection(srv, con, p);

	if (p->conf.rtsauth_require == NULL) return HANDLER_GO_ON;

	/*
	 * AUTH
	 *
	 */

	/* do we have to ask for rtsauth ? */

	rtsauth_required = 0;
	rtsauth_satisfied = 0;

	/* search rtsauth-directives for path */
	for (k = 0; k < p->conf.rtsauth_require->used; k++) {
		buffer *require = p->conf.rtsauth_require->data[k]->key;

		if (require->used == 0) continue;
		if (con->uri.path->used < require->used) continue;

		/* if we have a case-insensitive FS we have to lower-case the URI here too */

		if (con->conf.force_lowercase_filenames) {
			if (0 == strncasecmp(con->uri.path->ptr, require->ptr, require->used - 1)) {
				rtsauth_required = 1;
				break;
			}
		} else {
			if (0 == strncmp(con->uri.path->ptr, require->ptr, require->used - 1)) {
				rtsauth_required = 1;
				break;
			}
		}
	}

	/* nothing to do for us */
	if (rtsauth_required == 0) return HANDLER_GO_ON;

	req = ((data_array *)(p->conf.rtsauth_require->data[k]))->value;
	req_method = (data_string *)array_get_element(req, "method");

	if (0 == strcmp(req_method->value->ptr, "extern")) {
		/* require REMOTE_USER to be already set */
		if (NULL == (ds = (data_string *)array_get_element(con->environment, "REMOTE_USER"))) {
			con->http_status = 401;
			con->mode = DIRECT;
			return HANDLER_FINISHED;
		} else if (http_rtsauth_match_rules(srv, req, ds->value->ptr, NULL, NULL)) {
			log_error_write(srv, __FILE__, __LINE__, "s", "rules didn't match");
			con->http_status = 401;
			con->mode = DIRECT;
			return HANDLER_FINISHED;
		} else {
			return HANDLER_GO_ON;
		}
	}

	/* try to get Authorization-header */

	if (NULL != (ds = (data_string *)array_get_element(con->request.headers, "Authorization")) && ds->value->used) {
		char *rtsauth_realm;

		http_rtsauthorization = ds->value->ptr;

		/* parse auth-header */
		if (NULL != (rtsauth_realm = strchr(http_rtsauthorization, ' '))) {
			int rtsauth_type_len = rtsauth_realm - http_rtsauthorization;

			if ((rtsauth_type_len == 5) &&
			    (0 == strncasecmp(http_rtsauthorization, "Basic", rtsauth_type_len))) {
				rtsauth_type = "Basic";

				if (0 == strcmp(req_method->value->ptr, "basic")) {
					rtsauth_satisfied = http_rtsauth_basic_check(srv, con, p, req, rtsauth_realm+1);
				}
			} else if ((rtsauth_type_len == 6) &&
				   (0 == strncasecmp(http_rtsauthorization, "Digest", rtsauth_type_len))) {
				rtsauth_type = "Digest";
				if (0 == strcmp(req_method->value->ptr, "digest")) {
					if (-1 == (rtsauth_satisfied = http_rtsauth_digest_check(srv, con, p, req, rtsauth_realm+1))) {
						con->http_status = 400;
						con->mode = DIRECT;

						/* a field was missing */

						return HANDLER_FINISHED;
					}
				}
			} else {
				log_error_write(srv, __FILE__, __LINE__, "ss",
						"unknown rtsauthentification type:",
						http_rtsauthorization);
			}
		}
	}

	if (!rtsauth_satisfied) {
		data_string *method, *realm;
		method = (data_string *)array_get_element(req, "method");
		realm = (data_string *)array_get_element(req, "realm");

		con->http_status = 401;
		con->mode = DIRECT;

		if (0 == strcmp(method->value->ptr, "basic")) {
			buffer_copy_string_len(p->tmp_buf, CONST_STR_LEN("Basic realm=\""));
			buffer_append_string_buffer(p->tmp_buf, realm->value);
			buffer_append_string_len(p->tmp_buf, CONST_STR_LEN("\""));

			response_header_insert(srv, con, CONST_STR_LEN("WWW-Authenticate"), CONST_BUF_LEN(p->tmp_buf));
		} else if (0 == strcmp(method->value->ptr, "digest")) {
			char hh[33];
			http_rtsauth_digest_generate_nonce(srv, p, srv->tmp_buf, hh);

			buffer_copy_string_len(p->tmp_buf, CONST_STR_LEN("Digest realm=\""));
			buffer_append_string_buffer(p->tmp_buf, realm->value);
			buffer_append_string_len(p->tmp_buf, CONST_STR_LEN("\", nonce=\""));
			buffer_append_string(p->tmp_buf, hh);
			buffer_append_string_len(p->tmp_buf, CONST_STR_LEN("\", qop=\"auth\""));

			response_header_insert(srv, con, CONST_STR_LEN("WWW-Authenticate"), CONST_BUF_LEN(p->tmp_buf));
		} else {
			/* evil */
		}
		return HANDLER_FINISHED;
	} else {
		/* the REMOTE_USER header */

		if (NULL == (ds = (data_string *)array_get_element(con->environment, "REMOTE_USER"))) {
			if (NULL == (ds = (data_string *)array_get_unused_element(con->environment, TYPE_STRING))) {
				ds = data_string_init();
			}
			buffer_copy_string(ds->key, "REMOTE_USER");
			array_insert_unique(con->environment, (data_unset *)ds);
		}
		buffer_copy_string_buffer(ds->value, p->rtsauth_user);

		/* AUTH_TYPE environment */

		if (NULL == (ds = (data_string *)array_get_element(con->environment, "AUTH_TYPE"))) {
			if (NULL == (ds = (data_string *)array_get_unused_element(con->environment, TYPE_STRING))) {
				ds = data_string_init();
			}
			buffer_copy_string(ds->key, "AUTH_TYPE");
			array_insert_unique(con->environment, (data_unset *)ds);
		}
		buffer_copy_string(ds->value, rtsauth_type);
	}

	return HANDLER_GO_ON;
}

SETDEFAULTS_FUNC(mod_rtsauth_set_defaults) {
	mod_rtsauth_plugin_data *p = p_d;
	size_t i;

	printf("mod_rtsauth_set_defaults\n");
	config_values_t cv[] = {
		{ "rtsauth.backend",                   NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 0 */
		{ "rtsauth.backend.plain.groupfile",   NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 1 */
		{ "rtsauth.backend.plain.userfile",    NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 2 */
		{ "rtsauth.require",                   NULL, T_CONFIG_LOCAL, T_CONFIG_SCOPE_CONNECTION },  /* 3 */
		{ "rtsauth.backend.ldap.hostname",     NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 4 */
		{ "rtsauth.backend.ldap.base-dn",      NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 5 */
		{ "rtsauth.backend.ldap.filter",       NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 6 */
		{ "rtsauth.backend.ldap.ca-file",      NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 7 */
		{ "rtsauth.backend.ldap.starttls",     NULL, T_CONFIG_BOOLEAN, T_CONFIG_SCOPE_CONNECTION }, /* 8 */
		{ "rtsauth.backend.ldap.bind-dn",      NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 9 */
		{ "rtsauth.backend.ldap.bind-pw",      NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 10 */
		{ "rtsauth.backend.ldap.allow-empty-pw",     NULL, T_CONFIG_BOOLEAN, T_CONFIG_SCOPE_CONNECTION }, /* 11 */
		{ "rtsauth.backend.htdigest.userfile", NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 12 */
		{ "rtsauth.backend.htpasswd.userfile", NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION }, /* 13 */
		{ "rtsauth.debug",                     NULL, T_CONFIG_SHORT, T_CONFIG_SCOPE_CONNECTION },  /* 14 */
		{ NULL,                             NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
	};

	p->config_storage = calloc(1, srv->config_context->used * sizeof(mod_rtsauth_plugin_config *));

	for (i = 0; i < srv->config_context->used; i++) {
		mod_rtsauth_plugin_config *s;
		size_t n;
		data_array *da;
		array *ca;

		s = calloc(1, sizeof(mod_rtsauth_plugin_config));
		s->rtsauth_plain_groupfile = buffer_init();
		s->rtsauth_plain_userfile = buffer_init();
		s->rtsauth_htdigest_userfile = buffer_init();
		s->rtsauth_htpasswd_userfile = buffer_init();
		s->rtsauth_backend_conf = buffer_init();

		s->rtsauth_ldap_hostname = buffer_init();
		s->rtsauth_ldap_basedn = buffer_init();
		s->rtsauth_ldap_binddn = buffer_init();
		s->rtsauth_ldap_bindpw = buffer_init();
		s->rtsauth_ldap_filter = buffer_init();
		s->rtsauth_ldap_cafile = buffer_init();
		s->rtsauth_ldap_starttls = 0;
		s->rtsauth_debug = 0;

		s->rtsauth_require = array_init();

#ifdef USE_LDAP
		s->ldap_filter_pre = buffer_init();
		s->ldap_filter_post = buffer_init();
		s->ldap = NULL;
#endif

		cv[0].destination = s->rtsauth_backend_conf;
		cv[1].destination = s->rtsauth_plain_groupfile;
		cv[2].destination = s->rtsauth_plain_userfile;
		cv[3].destination = s->rtsauth_require;
		cv[4].destination = s->rtsauth_ldap_hostname;
		cv[5].destination = s->rtsauth_ldap_basedn;
		cv[6].destination = s->rtsauth_ldap_filter;
		cv[7].destination = s->rtsauth_ldap_cafile;
		cv[8].destination = &(s->rtsauth_ldap_starttls);
		cv[9].destination = s->rtsauth_ldap_binddn;
		cv[10].destination = s->rtsauth_ldap_bindpw;
		cv[11].destination = &(s->rtsauth_ldap_allow_empty_pw);
		cv[12].destination = s->rtsauth_htdigest_userfile;
		cv[13].destination = s->rtsauth_htpasswd_userfile;
		cv[14].destination = &(s->rtsauth_debug);

		p->config_storage[i] = s;
		ca = ((data_config *)srv->config_context->data[i])->value;

		if (0 != config_insert_values_global(srv, ca, cv)) {
			return HANDLER_ERROR;
		}

		if (s->rtsauth_backend_conf->used) {
			if (0 == strcmp(s->rtsauth_backend_conf->ptr, "htpasswd")) {
				s->rtsauth_backend = RTSAUTH_BACKEND_HTPASSWD;
			} else if (0 == strcmp(s->rtsauth_backend_conf->ptr, "htdigest")) {
				s->rtsauth_backend = RTSAUTH_BACKEND_HTDIGEST;
			} else if (0 == strcmp(s->rtsauth_backend_conf->ptr, "plain")) {
				s->rtsauth_backend = RTSAUTH_BACKEND_PLAIN;
			} else if (0 == strcmp(s->rtsauth_backend_conf->ptr, "ldap")) {
				s->rtsauth_backend = RTSAUTH_BACKEND_LDAP;
			} else {
				log_error_write(srv, __FILE__, __LINE__, "sb", "rtsauth.backend not supported:", s->rtsauth_backend_conf);

				return HANDLER_ERROR;
			}
		}

#ifdef USE_LDAP
		if (s->rtsauth_ldap_filter->used) {
			char *dollar;

			/* parse filter */

			if (NULL == (dollar = strchr(s->rtsauth_ldap_filter->ptr, '$'))) {
				log_error_write(srv, __FILE__, __LINE__, "s", "ldap: rtsauth.backend.ldap.filter is missing a replace-operator '$'");

				return HANDLER_ERROR;
			}

			buffer_copy_string_len(s->ldap_filter_pre, s->rtsauth_ldap_filter->ptr, dollar - s->rtsauth_ldap_filter->ptr);
			buffer_copy_string(s->ldap_filter_post, dollar+1);
		}
#endif

		/* no rtsauth.require for this section */
		if (NULL == (da = (data_array *)array_get_element(ca, "rtsauth.require"))) continue;

		if (da->type != TYPE_ARRAY) continue;

		for (n = 0; n < da->value->used; n++) {
			size_t m;
			data_array *da_file = (data_array *)da->value->data[n];
			const char *method, *realm, *require;

			if (da->value->data[n]->type != TYPE_ARRAY) {
				log_error_write(srv, __FILE__, __LINE__, "ss",
						"rtsauth.require should contain an array as in:",
						"rtsauth.require = ( \"...\" => ( ..., ...) )");

				return HANDLER_ERROR;
			}

			method = realm = require = NULL;

			for (m = 0; m < da_file->value->used; m++) {
				if (da_file->value->data[m]->type == TYPE_STRING) {
					if (0 == strcmp(da_file->value->data[m]->key->ptr, "method")) {
						method = ((data_string *)(da_file->value->data[m]))->value->ptr;
					} else if (0 == strcmp(da_file->value->data[m]->key->ptr, "realm")) {
						realm = ((data_string *)(da_file->value->data[m]))->value->ptr;
					} else if (0 == strcmp(da_file->value->data[m]->key->ptr, "require")) {
						require = ((data_string *)(da_file->value->data[m]))->value->ptr;
					} else {
						log_error_write(srv, __FILE__, __LINE__, "ssbs",
							"the field is unknown in:",
							"rtsauth.require = ( \"...\" => ( ..., -> \"",
							da_file->value->data[m]->key,
							"\" <- => \"...\" ) )");

						return HANDLER_ERROR;
					}
				} else {
					log_error_write(srv, __FILE__, __LINE__, "ssbs",
						"a string was expected for:",
						"rtsauth.require = ( \"...\" => ( ..., -> \"",
						da_file->value->data[m]->key,
						"\" <- => \"...\" ) )");

					return HANDLER_ERROR;
				}
			}

			if (method == NULL) {
				log_error_write(srv, __FILE__, __LINE__, "ss",
						"the method field is missing in:",
						"rtsauth.require = ( \"...\" => ( ..., \"method\" => \"...\" ) )");
				return HANDLER_ERROR;
			} else {
				if (0 != strcmp(method, "basic") &&
				    0 != strcmp(method, "digest") &&
				    0 != strcmp(method, "extern")) {
					log_error_write(srv, __FILE__, __LINE__, "ss",
							"method has to be either \"basic\", \"digest\" or \"extern\" in",
							"rtsauth.require = ( \"...\" => ( ..., \"method\" => \"...\") )");
					return HANDLER_ERROR;
				}
			}

			if (realm == NULL) {
				log_error_write(srv, __FILE__, __LINE__, "ss",
						"the realm field is missing in:",
						"rtsauth.require = ( \"...\" => ( ..., \"realm\" => \"...\" ) )");
				return HANDLER_ERROR;
			}

			if (require == NULL) {
				log_error_write(srv, __FILE__, __LINE__, "ss",
						"the require field is missing in:",
						"rtsauth.require = ( \"...\" => ( ..., \"require\" => \"...\" ) )");
				return HANDLER_ERROR;
			}

			if (method && realm && require) {
				data_string *ds;
				data_array *a;

				a = data_array_init();
				buffer_copy_string_buffer(a->key, da_file->key);

				ds = data_string_init();

				buffer_copy_string_len(ds->key, CONST_STR_LEN("method"));
				buffer_copy_string(ds->value, method);

				array_insert_unique(a->value, (data_unset *)ds);

				ds = data_string_init();

				buffer_copy_string_len(ds->key, CONST_STR_LEN("realm"));
				buffer_copy_string(ds->value, realm);

				array_insert_unique(a->value, (data_unset *)ds);

				ds = data_string_init();

				buffer_copy_string_len(ds->key, CONST_STR_LEN("require"));
				buffer_copy_string(ds->value, require);

				array_insert_unique(a->value, (data_unset *)ds);

				array_insert_unique(s->rtsauth_require, (data_unset *)a);
			}
		}

		switch(s->rtsauth_ldap_hostname->used) {
		case RTSAUTH_BACKEND_LDAP: {
			handler_t ret = rtsauth_ldap_init(srv, s);
			if (ret == HANDLER_ERROR)
				return (ret);
			break;
		}
		default:
			break;
		}
	}

	return HANDLER_GO_ON;
}

handler_t rtsauth_ldap_init(server *srv, mod_rtsauth_plugin_config *s) {
#ifdef USE_LDAP
	int ret;
#if 0
	if (s->rtsauth_ldap_basedn->used == 0) {
		log_error_write(srv, __FILE__, __LINE__, "s", "ldap: rtsauth.backend.ldap.base-dn has to be set");

		return HANDLER_ERROR;
	}
#endif

	log_error_write(srv, __FILE__, __LINE__, "s", "rtsauth_ldap_init");

	if (s->rtsauth_ldap_hostname->used) {
		/* free old context */
		if (NULL != s->ldap) ldap_unbind_s(s->ldap);

		if (NULL == (s->ldap = ldap_init(s->rtsauth_ldap_hostname->ptr, LDAP_PORT))) {
			log_error_write(srv, __FILE__, __LINE__, "ss", "ldap ...", strerror(errno));

			return HANDLER_ERROR;
		}

		ret = LDAP_VERSION3;
		if (LDAP_OPT_SUCCESS != (ret = ldap_set_option(s->ldap, LDAP_OPT_PROTOCOL_VERSION, &ret))) {
			log_error_write(srv, __FILE__, __LINE__, "ss", "ldap:", ldap_err2string(ret));

			return HANDLER_ERROR;
		}

		if (s->rtsauth_ldap_starttls) {
			/* if no CA file is given, it is ok, as we will use encryption
				* if the server requires a CAfile it will tell us */
			if (!buffer_is_empty(s->rtsauth_ldap_cafile)) {
				if (LDAP_OPT_SUCCESS != (ret = ldap_set_option(NULL, LDAP_OPT_X_TLS_CACERTFILE,
								s->rtsauth_ldap_cafile->ptr))) {
					log_error_write(srv, __FILE__, __LINE__, "ss",
							"Loading CA certificate failed:", ldap_err2string(ret));

					return HANDLER_ERROR;
				}
			}

			if (LDAP_OPT_SUCCESS != (ret = ldap_start_tls_s(s->ldap, NULL,  NULL))) {
				log_error_write(srv, __FILE__, __LINE__, "ss", "ldap startTLS failed:", ldap_err2string(ret));

				return HANDLER_ERROR;
			}
		}


		/* 1. */
		if (s->rtsauth_ldap_binddn->used) {
			if (LDAP_SUCCESS != (ret = ldap_simple_bind_s(s->ldap, s->rtsauth_ldap_binddn->ptr, s->rtsauth_ldap_bindpw->ptr))) {
				log_error_write(srv, __FILE__, __LINE__, "ss", "ldap:", ldap_err2string(ret));

				return HANDLER_ERROR;
			}
		} else {
			if (LDAP_SUCCESS != (ret = ldap_simple_bind_s(s->ldap, NULL, NULL))) {
				log_error_write(srv, __FILE__, __LINE__, "ss", "ldap:", ldap_err2string(ret));

				return HANDLER_ERROR;
			}
		}
	}
	return HANDLER_GO_ON;
#else
	UNUSED(s);
	log_error_write(srv, __FILE__, __LINE__, "s", "no ldap support available");
	return HANDLER_ERROR;
#endif
}

int mod_rtsauth_plugin_init(plugin *p);
int mod_rtsauth_plugin_init(plugin *p) {
	printf("mod_rtsauth_plugin_init\n");
	p->version     = LIGHTTPD_VERSION_ID;
	p->name        = buffer_init_string("rtsauth");
	p->init        = mod_rtsauth_init;
	p->set_defaults = mod_rtsauth_set_defaults;
	p->handle_uri_clean = mod_rtsauth_uri_handler;
	p->cleanup     = mod_rtsauth_free;

	p->data        = NULL;

	return 0;
}
