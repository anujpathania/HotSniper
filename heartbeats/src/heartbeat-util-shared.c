/**
 * Functions defined in heartbeat header files that are reusable across
 * implementations due to common structure in heartbeat_t structs.
 *
 * To disable function definitions for a particular heartbeat interface and
 * implement them elsewhere, define the following macros as needed:
 *   HEARTBEAT_UTIL_OVERRIDE
 *   HEARTBEAT_ACCURACY_UTIL_OVERRIDE
 *   HEARTBEAT_ACCURACY_POWER_UTIL_OVERRIDE
 *
 * @author Connor Imes
 * @author Hank Hoffmann
 */

#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "heartbeat-util-shared.h"
/* The proper heartbeat implementation to include is done so in the header */

/*
 * First we have internal utility functions
 */

/**
 *
 * @param pid integer
 */
_HB_global_state_t* HB_alloc_state(int pid) {
  _HB_global_state_t* p = NULL;
  int shmid;

  shmid = shmget((pid << 1) | 1, 1*sizeof(_HB_global_state_t), IPC_CREAT | 0666);
  if (shmid < 0) {
    perror("cannot allocate shared memory for heartbeat global state");
    return NULL;
  }

  /*
   * Now we attach the segment to our data space.
   */
  p = (_HB_global_state_t*) shmat(shmid, NULL, 0);
  if (p == (_HB_global_state_t*) -1) {
    perror("cannot attach shared memory to heartbeat global state");
    return NULL;
  }

  return p;
}

/**
 * Helper function for allocating shared memory
 *
 * @param pid integer
 * @param buffer_size int64_t
 */
_heartbeat_record_t* HB_alloc_log(int pid, int64_t buffer_size) {
  _heartbeat_record_t* p = NULL;
  int shmid;

  printf("Allocating log for %d, %d\n", pid, pid << 1);

  shmid = shmget(pid << 1, buffer_size*sizeof(_heartbeat_record_t), IPC_CREAT | 0666);
  if (shmid < 0) {
    perror("cannot allocate shared memory for heartbeat records");
    return NULL;
  }

  /*
   * Now we attach the segment to our data space.
   */
  p = (_heartbeat_record_t*) shmat(shmid, NULL, 0);
  if (p == (_heartbeat_record_t*) -1) {
    perror("cannot attach shared memory to heartbeat enabled process");
    return NULL;
  }

  return p;
}

/*
 * Functions from heartbeat.h
 */
#if !defined(HEARTBEAT_UTIL_OVERRIDE)

int64_t hb_get_window_size(heartbeat_t volatile * hb) {
  return hb->state->window_size;
}

int64_t hb_get_buffer_depth(heartbeat_t volatile * hb) {
  return hb->state->buffer_depth;
}

void hb_get_current(heartbeat_t volatile * hb,
                    heartbeat_record_t volatile * record) {
  hb_get_history(hb, record, 1);
}

int64_t hb_get_history(heartbeat_t volatile * hb,
                       heartbeat_record_t volatile * record,
                       int64_t n) {
  if (n <= 0) {
    return 0;
  }

  if (n > hb->state->counter) {
    // more records were requested than have been created
    memcpy(record,
           &hb->log[0],
           (size_t)hb->state->buffer_index * sizeof(heartbeat_record_t));
    return hb->state->buffer_index;
  }

  if (hb->state->buffer_index >= n) {
    // the number of records requested do not overflow the circular buffer
    memcpy(record,
           &hb->log[hb->state->buffer_index - n],
		   (size_t)n * sizeof(heartbeat_record_t));
    return n;
  }

  // the number of records requested could overflow the circular buffer
  if (n >= hb->state->buffer_depth) {
    // more records were requested than we can support, return what we have
    memcpy(record,
         &hb->log[hb->state->buffer_index],
	     (size_t)(hb->state->buffer_depth - hb->state->buffer_index) * sizeof(heartbeat_record_t));
    memcpy(record + hb->state->buffer_depth - hb->state->buffer_index,
           &hb->log[0],
	     (size_t)hb->state->buffer_index * sizeof(heartbeat_record_t));
    return hb->state->buffer_depth;
  }

  // buffer_index < n < buffer_depth
  // still overflows circular buffer, but we don't want all records
  memcpy(record,
         &hb->log[hb->state->buffer_depth + hb->state->buffer_index - n],
         (size_t)(n - hb->state->buffer_index) * sizeof(heartbeat_record_t));
  memcpy(record + n - hb->state->buffer_index,
         &hb->log[0],
         (size_t)hb->state->buffer_index * sizeof(heartbeat_record_t));
  return n;
}

double hb_get_min_rate(heartbeat_t volatile * hb) {
  return hb->state->min_heartrate;
}

double hb_get_max_rate(heartbeat_t volatile * hb) {
  return hb->state->max_heartrate;
}

double hb_get_global_rate(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].global_rate;
}

double hb_get_windowed_rate(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].window_rate;
}

double hb_get_instant_rate(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].instant_rate;
}

int64_t hbr_get_beat_number(heartbeat_record_t volatile * hbr) {
  return hbr->beat;
}

int hbr_get_tag(heartbeat_record_t volatile * hbr) {
  return hbr->tag;
}

int64_t hbr_get_timestamp(heartbeat_record_t volatile * hbr) {
  return hbr->timestamp;
}

double hbr_get_global_rate(heartbeat_record_t volatile * hbr) {
  return hbr->global_rate;
}

double hbr_get_window_rate(heartbeat_record_t volatile * hbr) {
  return hbr->window_rate;
}

double hbr_get_instant_rate(heartbeat_record_t volatile * hbr) {
  return hbr->instant_rate;
}

#endif

/*
 * Functions from heartbeat-accuracy.h
 */
#if defined(HEARTBEAT_MODE_ACC) && !defined(HEARTBEAT_ACCURACY_UTIL_OVERRIDE)

double hb_get_min_accuracy(heartbeat_t volatile * hb) {
  return hb->state->min_accuracy;
}

double hb_get_max_accuracy(heartbeat_t volatile * hb) {
  return hb->state->max_accuracy;
}

double hb_get_global_accuracy(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].global_accuracy;
}

double hb_get_windowed_accuracy(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].window_accuracy;
}

double hb_get_instant_accuracy(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].instant_accuracy;
}

double hbr_get_global_accuracy(heartbeat_record_t volatile * hbr) {
  return hbr->global_accuracy;
}

double hbr_get_window_accuracy(heartbeat_record_t volatile * hbr) {
  return hbr->window_accuracy;
}

double hbr_get_instant_accuracy(heartbeat_record_t volatile * hbr) {
  return hbr->instant_accuracy;
}

#endif

/*
 * Functions from heartbeat-accuracy-power.h
 */
#if defined(HEARTBEAT_MODE_ACC_POW) && !defined(HEARTBEAT_ACCURACY_POWER_UTIL_OVERRIDE)

double hb_get_min_power(heartbeat_t volatile * hb) {
  return hb->state->min_power;
}

double hb_get_max_power(heartbeat_t volatile * hb) {
  return hb->state->max_power;
}

double hb_get_global_power(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].global_power;
}

double hb_get_windowed_power(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].window_power;
}

double hb_get_instant_power(heartbeat_t volatile * hb) {
  return hb->log[hb->state->read_index].instant_power;
}

double hbr_get_global_power(heartbeat_record_t volatile * hbr) {
  return hbr->global_power;
}

double hbr_get_window_power(heartbeat_record_t volatile * hbr) {
  return hbr->window_power;
}

double hbr_get_instant_power(heartbeat_record_t volatile * hbr) {
  return hbr->instant_power;
}

#endif
