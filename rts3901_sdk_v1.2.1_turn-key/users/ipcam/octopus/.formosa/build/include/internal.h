#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#include "hw_control.h"

int opt_get_msg_id(const char *file);
int opt_alloc_cmd_message(struct cmd_msg **__cmd, int len);
void opt_free_cmd_msg(struct cmd_msg **cmd);
int opt_send_cmd_msg(int msg, struct cmd_msg *cmd, int _timeout);

#endif
