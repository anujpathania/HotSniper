/**
 * An extension of the capabilities in "heartbeat.h" to include monitoring
 * program accuracy.
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _HEARTBEAT_ACCURACY_H_
#define _HEARTBEAT_ACCURACY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "heartbeat-accuracy-types.h"
#include "heartbeat.h"

/**
 * Registers a heartbeat
 *
 * @param hb pointer to heartbeat_t
 * @param tag integer
 * @param accuracy double
 */
int64_t heartbeat_acc(heartbeat_t* hb,
                      int tag,
                      double accuracy);

/**
 * Returns the minimum desired accuracy
 *
 * @param hb pointer to heartbeat_t
 * @return the minimum desired accuracy (double)
 */
double hb_get_min_accuracy(heartbeat_t volatile * hb);

/**
 * Returns the maximum desired accuracy
 *
 * @param hb pointer to heartbeat_t
 * @return the maximum desired accuracy (double)
 */
double hb_get_max_accuracy(heartbeat_t volatile * hb);

/**
 * Returns the accuracy over the life of the entire application
 *
 * @param hb pointer to heartbeat_t
 * @return the accuracy (double) over the entire life of the application
 */
double hb_get_global_accuracy(heartbeat_t volatile * hb);

/**
 * Returns the accuracy over the last window (as specified to init)
 * heartbeats
 *
 * @param hb pointer to heartbeat_t
 * @return the accuracy (double) over the last window
 */
double hb_get_windowed_accuracy(heartbeat_t volatile * hb);

/**
 * Returns the accuracy for the last heartbeat.
 *
 * @param hb pointer to heartbeat_t
 * @return the accuracy (double) for the last heartbeat
 */
double hb_get_instant_accuracy(heartbeat_t volatile * hb);

/**
 * Returns the global accuracy recorded in this record.
 *
 * @param hbr
 */
double hbr_get_global_accuracy(heartbeat_record_t volatile * hbr);

/**
 * Returns the window accuracy recorded in this record.
 *
 * @param hbr
 */
double hbr_get_window_accuracy(heartbeat_record_t volatile * hbr);

/**
 * Returns the instant accuracy recorded in this record.
 *
 * @param hbr
 */
double hbr_get_instant_accuracy(heartbeat_record_t volatile * hbr);

#ifdef __cplusplus
}
#endif

#endif
