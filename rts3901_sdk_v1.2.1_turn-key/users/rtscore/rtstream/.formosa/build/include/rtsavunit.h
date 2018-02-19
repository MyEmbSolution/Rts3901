/*
 * Realtek Semiconductor Corp.
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _CORE_RTSAVUNIT_H
#define _CORE_RTSAVUNIT_H

#include <stdint.h>
#include <rtsavapi.h>

enum rts_av_unit_type {
	unit_type_none = 0x0,
	unit_type_source = 0x1,
	unit_type_sink = 0x2,
	unit_type_filter = 0x3,
};

struct rts_av_unit {
	enum rts_av_unit_type (*get_type)();
	unsigned int (*get_id)();

	int (*init)(struct rts_av_unit *unit);
	int (*cleanup)(struct rts_av_unit *unit);

	int (*start)(struct rts_av_unit *unit);
	int (*stop)(struct rts_av_unit *unit);
	int (*runfunc)(struct rts_av_unit *unit);

	int (*set_attr)(struct rts_av_unit *unit, void *attr);
	int (*get_attr)(struct rts_av_unit *unit, void *attr);

	int (*get_profile)(struct rts_av_unit *unit,
			struct rts_av_profile *profile);
	int (*set_profile)(struct rts_av_unit *unit,
			struct rts_av_profile *profile);

	int (*exec_command)(struct rts_av_unit *unit, void *p);
	int (*query_ability)(struct rts_av_unit *unit,
			struct rts_av_ability_t *ability);
	int (*keep_active)();

	void *priv;
};

int rts_av_register_unit(struct rts_av_unit *unit);
int rts_av_unregister_unit(unsigned int id);

int rts_av_poll_input_buffer(struct rts_av_unit *unit);
int rts_av_pop_input_buffer(struct rts_av_unit *unit, struct rts_av_buffer **ppbuf);
int rts_av_push_back_output_buffer(struct rts_av_unit *unit, struct rts_av_buffer *pbuf);

void rts_av_set_unit_rotation(struct rts_av_unit *unit, int rotation);
int rts_av_get_unit_rotation(struct rts_av_unit *unit);

int rts_av_unit_keep_active(struct rts_av_unit *unit);

#endif
