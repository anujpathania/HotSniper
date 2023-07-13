/**
 * Heartbeats API for monitoring program performance.
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "heartbeat-types.h"
#include <stdint.h>

/**
 * Initialization function for process that
 * wants to register heartbeats
 *
 * @param window_size int64_t
 * @param buffer_depth int64_t
 * @param log_name pointer to char
 * @param min_target double
 * @param max_target double
 */
heartbeat_t* heartbeat_init(int64_t window_size,
                            int64_t buffer_depth,
                            const char* log_name,
                            double min_target,
                            double max_target);

/**
 * Registers a heartbeat
 *
 * @param hb pointer to heartbeat_t
 * @param tag integer
 */
int64_t heartbeat(heartbeat_t* hb,
                  int tag);

/**
 * Cleanup function for process that
 * wants to register heartbeats
 *
 * @param hb pointer to heartbeat_t
 */
void heartbeat_finish(heartbeat_t * hb);

/**
 * Returns the size of the sliding window used to compute the current heart
 * rate
 *
 * @param hb pointer to heartbeat_t
 * @return the size of the sliding window (int64_t)
 */
int64_t hb_get_window_size(heartbeat_t volatile * hb);

/**
 * Returns the buffer depth of the log
 * rate
 *
 * @param hb pointer to heartbeat_t
 * @return the buffer depth (int64_t)
 */
int64_t hb_get_buffer_depth(heartbeat_t volatile * hb);

/**
 * Returns the record for the current heartbeat
 * currently may read old data
 *
 * @param hb pointer to heartbeat_t
 */
void hb_get_current(heartbeat_t volatile * hb,
                    heartbeat_record_t volatile * record);

/**
 * Returns all heartbeat information for the last n heartbeats
 *
 * @param hb pointer to heartbeat_t
 * @param record pointer to heartbeat_record_t
 * @param n int64_t
 */
int64_t hb_get_history(heartbeat_t volatile * hb,
                       heartbeat_record_t volatile * record,
                       int64_t n);

/**
 * Returns the minimum desired heart rate
 *
 * @param hb pointer to heartbeat_t
 * @return the minimum desired heart rate (double)
 */
double hb_get_min_rate(heartbeat_t volatile * hb);

/**
 * Returns the maximum desired heart rate
 *
 * @param hb pointer to heartbeat_t
 * @return the maximum desired heart rate (double)
 */
double hb_get_max_rate(heartbeat_t volatile * hb);

/**
 * Returns the heart rate over the life of the entire application
 *
 * @param hb pointer to heartbeat_t
 * @return the heart rate (double) over the entire life of the application
 */
double hb_get_global_rate(heartbeat_t volatile * hb);

/**
 * Returns the heart rate over the last window (as specified to init)
 * heartbeats
 *
 * @param hb pointer to heartbeat_t
 * @return the heart rate (double) over the last window
 */
double hb_get_windowed_rate(heartbeat_t volatile * hb);

/**
 * Returns the heart rate for the last heartbeat.
 *
 * @param hb pointer to heartbeat_t
 * @return the heart rate (double) for the last heartbeat
 */
double hb_get_instant_rate(heartbeat_t volatile * hb);

/**
 * Returns the heartbeat number for this record.
 *
 * @param hbr
 */
int64_t hbr_get_beat_number(heartbeat_record_t volatile * hbr);

/**
 * Returns the tag number for this record.
 *
 * @param hbr
 */
int hbr_get_tag(heartbeat_record_t volatile * hbr);

/**
 * Returns the timestamp for this record.
 *
 * @param hbr
 */
int64_t hbr_get_timestamp(heartbeat_record_t volatile * hbr);

/**
 * Returns the global rate recorded in this record.
 *
 * @param hbr
 */
double hbr_get_global_rate(heartbeat_record_t volatile * hbr);

/**
 * Returns the window rate recorded in this record.
 *
 * @param hbr
 */
double hbr_get_window_rate(heartbeat_record_t volatile * hbr);

/**
 * Returns the instante rate recorded in this record.
 *
 * @param hbr
 */
double hbr_get_instant_rate(heartbeat_record_t volatile * hbr);

#ifdef __cplusplus
 }
#endif

#endif
