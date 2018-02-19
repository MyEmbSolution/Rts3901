/*
 * neuralyzer.h
 *
 * Copyright(C) 2015 Micky Ching, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NEURALYZER_H
#define NEURALYZER_H /* NEURALYZER_H */

#include <sys/types.h>

/**
 * neuralyzer is a linux image burning tool library,
 * which support burn by arbitrary segment split.
 * Two burn mode is supported:
 * 1. file mode is the easiest way to use neuralyzer.
 *    include 5 step:
 *    a. neu_data_alloc() - alloc data by provide a filename.
 *    b. neu_check() - check if the file is valid.
 *    c. neu_prepare() - prepare for burning.
 *    d. neu_burn() - burning linux image.
 *    e. neu_data_free() - free data.
 * 2. iter mode is much complicate, this is also the base of file mode.
 *    a. neu_data_alloc() - alloc data by provide a NULL pointer
 *    b. neu_check_iter() - check image,
 *       please refer neu_loop() for how to use iter function.
 *    c. neu_prepare() - preprare for buring.
 *    d. neu_burn_iter() - burning linux image, refer neu_loop()
 *    e. neu_data_free() - free data.
 */
struct neu_data;

struct neu_data *neu_data_alloc(const char *filename);
void neu_data_free(struct neu_data *data);
void neu_data_dump(const struct neu_data *data);

int neu_erase_node(const char *name);
int neu_write_node(const char *name, const void *buffer, int len);

int neu_burn_percent(const struct neu_data *data);
int neu_burn_finish(const struct neu_data *data);
int neu_burn_pass(const struct neu_data *data);

int neu_check_iter(struct neu_data *data, const void *buffer, int len);
int neu_check(struct neu_data *data);

int neu_prepare(struct neu_data *data);

int neu_burn_iter(struct neu_data *data, const void *buffer, int len);
int neu_burn(struct neu_data *data);

int neu_burn_schedule(struct neu_data *data);
#endif /* NEURALYZER_H */
