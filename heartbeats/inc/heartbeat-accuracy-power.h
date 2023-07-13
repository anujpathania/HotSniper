/**
 * An extension of the capabilities in "heartbeat-accuracy.h" to include
 * monitoring system power/energy usage.
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _HEARTBEAT_ACCURACY_POWER_H_
#define _HEARTBEAT_ACCURACY_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "heartbeat-accuracy-power-types.h"
#include "heartbeat-accuracy.h"
#include "hb-energy.h"
#include <stdint.h>

/**
 * Initialization function for process that wants to register heartbeats
 *
 * @param window_size int64_t
 * @param buffer_depth int64_t
 * @param log_name pointer to char
 * @param min_perf double
 * @param max_perf double
 * @param min_acc double
 * @param max_acc double
 * @param num_energy_impls
 * @param energy_impls
 * @param min_pow double
 * @param max_pow double
 */
heartbeat_t* heartbeat_acc_pow_init(int64_t window_size,
                                    int64_t buffer_depth,
                                    const char* log_name,
                                    double min_perf,
                                    double max_perf,
                                    double min_acc,
                                    double max_acc,
                                    uint64_t num_energy_impls,
                                    hb_energy_impl* energy_impls,
                                    double min_pow,
                                    double max_pow);

/**
 * Returns the minimum desired power
 *
 * @param hb pointer to heartbeat_t
 * @return the minimum desired power (double)
 */
double hb_get_min_power(heartbeat_t volatile * hb);

/**
 * Returns the maximum desired power
 *
 * @param hb pointer to heartbeat_t
 * @return the maximum desired power (double)
 */
double hb_get_max_power(heartbeat_t volatile * hb);

/**
 * Returns the power over the life of the entire application
 *
 * @param hb pointer to heartbeat_t
 * @return the power (double) over the entire life of the application
 */
double hb_get_global_power(heartbeat_t volatile * hb);

/**
 * Returns the power over the last window (as specified to init)
 * heartbeats
 *
 * @param hb pointer to heartbeat_t
 * @return the power (double) over the last window
 */
double hb_get_windowed_power(heartbeat_t volatile * hb);

/**
 * Returns the power for the last heartbeat.
 *
 * @param hb pointer to heartbeat_t
 * @return the power (double) for the last heartbeat
 */
double hb_get_instant_power(heartbeat_t volatile * hb);

/**
 * Returns the global power recorded in this record.
 *
 * @param hbr
 */
double hbr_get_global_power(heartbeat_record_t volatile * hbr);

/**
 * Returns the window power recorded in this record.
 *
 * @param hbr
 */
double hbr_get_window_power(heartbeat_record_t volatile * hbr);

/**
 * Returns the instant power recorded in this record.
 *
 * @param hbr
 */
double hbr_get_instant_power(heartbeat_record_t volatile * hbr);

#ifdef __cplusplus
}
#endif

#endif
