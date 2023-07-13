/** \file
 *  \brief
 *  \author Hank Hoffmann
 *  \version 1.0
 */

#include "heart_rate_monitor.h"
#include "heartbeat-types.h"
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

/**
       *
       * @param hrm pointer to heart_rate_monitor_t
       * @param pid integer
       */
int heart_rate_monitor_init(heart_rate_monitor_t* hrm,
			    int pid) {
  int shmid1;
  int shmid2;
  key_t key;
  int rc = 0;

  key = pid;
  printf("Attaching mem %d, %d\n", pid, key);

    if((shmid1 = shmget(((key<<1)|1), 1*sizeof(HB_global_state_t), 0666)) < 0) {
      rc = 1;
  }

  if ((hrm->state = (HB_global_state_t*) shmat(shmid1, NULL, 0)) == (HB_global_state_t*) -1) {
    rc = 1;
  }

  if(rc != 0)
    return rc;

#if 1
  if((shmid2 = shmget(((key<<1)), hrm->state->buffer_depth*sizeof(heartbeat_record_t), 0666)) < 0) {
    rc = 2;
  }

  if ((hrm->log = (heartbeat_record_t*) shmat(shmid2, NULL, 0)) == (heartbeat_record_t*) -1) {
    rc = 2;
  }
#endif

  if(rc != 0)
    printf("Couldn't get at shared mem %d\n", rc);

  return rc;
}

/**
       *
       * @param heart pointer to heart_rate_monitor_t
       */
void heart_rate_monitor_finish(heart_rate_monitor_t* heart) {

}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @param record pointer to heartbeat_record_t
       */
int hrm_get_current(heart_rate_monitor_t volatile * hb,
		     heartbeat_record_t volatile * record) {
  //memcpy(record, &hb->log[hb->state->read_index], sizeof(heartbeat_record_t));
    if(hb->state->valid) {
      memcpy(record,
	     &hb->log[hb->state->read_index],
	     sizeof(heartbeat_record_t));
    }

    return !hb->state->valid;
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @param record pointer to heartbeat_record_t
       */
int hrm_get_history(heart_rate_monitor_t volatile * hb,
		     heartbeat_record_t volatile * record,
		     int n) {
  if(hb->state->counter > hb->state->buffer_index) {
     memcpy(record,
	    &hb->log[hb->state->buffer_index],
	    (size_t)(hb->state->buffer_index*hb->state->buffer_depth)*sizeof(heartbeat_record_t));
     memcpy(record + (hb->state->buffer_index*hb->state->buffer_depth),
	    &hb->log[0],
	    (size_t)(hb->state->buffer_index)*sizeof(heartbeat_record_t));
     return (int)hb->state->buffer_depth;
  }
  else {
    memcpy(record,
	   &hb->log[0],
	   (size_t)hb->state->buffer_index*sizeof(heartbeat_record_t));
    return (int)hb->state->buffer_index;
  }
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @return double
       */
double hrm_get_global_rate(heart_rate_monitor_t volatile * hb) {
  return hb->log[hb->state->counter].global_rate;
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @return double
       */
double hrm_get_windowed_rate(heart_rate_monitor_t volatile * hb) {
  return hb->log[hb->state->counter].window_rate;
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @return double
       */
double hrm_get_min_rate(heart_rate_monitor_t volatile * hb) {
  return hb->state->min_heartrate;
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @return double
       */
double hrm_get_max_rate(heart_rate_monitor_t volatile * hb) {
  return hb->state->max_heartrate;
}

/**
       *
       * @param hb pointer to heart_rate_monitor_t
       * @return double
       */
int64_t hrm_get_window_size(heart_rate_monitor_t volatile * hb) {
  return hb->state->window_size;
}

