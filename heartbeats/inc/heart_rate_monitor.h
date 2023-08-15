#ifndef _HEARTRATEMONITOR_H_
#define _HEARTRATEMONITOR_H_

#include "heartbeat.h"
#include <stdio.h>

typedef struct {

  HB_global_state_t* state;
  heartbeat_record_t* log;
  FILE* file;
  char filename[256];

} heart_rate_monitor_t;

int heart_rate_monitor_init(heart_rate_monitor_t* hrm,
			    int pid);

void heart_rate_monitor_finish(heart_rate_monitor_t* heart);

int hrm_get_current(heart_rate_monitor_t volatile * hb,
		    heartbeat_record_t volatile * record);

int hrm_get_history(heart_rate_monitor_t volatile * hb,
		    heartbeat_record_t volatile * record,
		    int n);

double hrm_get_global_rate(heart_rate_monitor_t volatile * hb);

double hrm_get_windowed_rate(heart_rate_monitor_t volatile * hb);

double hrm_get_min_rate(heart_rate_monitor_t volatile * hb);

double hrm_get_max_rate(heart_rate_monitor_t volatile * hb);

int64_t hrm_get_window_size(heart_rate_monitor_t volatile * hb);

#endif
