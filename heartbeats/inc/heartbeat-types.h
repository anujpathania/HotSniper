/**
 * Heartbeat API type definitions.
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _HEARTBEAT_TYPES_H_
#define _HEARTBEAT_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
  int64_t beat;
  int tag;
  int64_t timestamp;

  double global_rate;
  double window_rate;
  double instant_rate;
} _heartbeat_record_t;

typedef struct {
  int pid;
  int64_t window_size;

  int64_t counter;
  int64_t buffer_depth;
  int64_t buffer_index;
  int64_t read_index;
  char    valid;

  double min_heartrate;
  double max_heartrate;
} _HB_global_state_t;

typedef struct {
  int64_t first_timestamp;
  int64_t last_timestamp;
  int steady_state;

  _heartbeat_record_t* log;

  FILE* binary_file;
  FILE* text_file;
  char filename[256];
  pthread_mutex_t mutex;

  _HB_global_state_t* state;

  int64_t* window;
  int64_t current_index;
  double last_average_time;
} _heartbeat_t;

typedef _heartbeat_record_t heartbeat_record_t;
typedef _HB_global_state_t HB_global_state_t;
typedef _heartbeat_t heartbeat_t;

#ifdef __cplusplus
}
#endif

#endif
