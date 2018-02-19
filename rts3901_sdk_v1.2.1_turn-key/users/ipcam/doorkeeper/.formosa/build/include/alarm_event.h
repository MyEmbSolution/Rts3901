#ifndef _ALARM_EVENT_H_
#define _ALARM_EVENT_H_

#include "alarm_schedule.h"
#include "alarm_action.h"


#define MAX_EVENT_ENTRY_SIZE		32
#define MAX_EVENT_ENTRY_NAME_SIZE	32


#define EVENT_NAME_MD	"MD"
#define EVENT_NAME_SCHEDULE	"SCHEDULE"


#define EVENT_ID_BASE			0x10000
#define EVENT_ID_MD_ALARM		(EVENT_ID_BASE + 1)
#define EVENT_ID_SCHEDULE_ALARM	(EVENT_ID_BASE + 2)


struct s_event_entry {
	char name[MAX_EVENT_ENTRY_NAME_SIZE];
	unsigned int id;
};

struct s_method_entry {
	char name[MAX_EVENT_ENTRY_NAME_SIZE];
	char libpath[64];
	char plugin_name[32];
};

int add_event_entry(char *event_name, unsigned int event_id);
void init_event_entry();
int set_eventid(unsigned int *event_id, const char event_name[]);
int set_schedule(struct s_schedule *schedule, struct s_event_schedule_setting *setting);
int set_actions(struct s_action_list *action_list, const char actions_str[]);


#endif

