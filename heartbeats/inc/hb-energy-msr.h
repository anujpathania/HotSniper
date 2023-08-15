/**
 * Read energy from X86 MSRs (Model-Specific Registers).
 *
 * By default, the MSR on cpu0 is read. To configure other MSRs, set the
 * HEARTBEAT_ENERGY_MSRS environment variable with a comma-delimited list of
 * CPU IDs, e.g.:
 *   export HEARTBEAT_ENERGY_MSRS=0,4,8,12
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _HB_ENERGY_MSR_H_
#define _HB_ENERGY_MSR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hb-energy.h"
#include <inttypes.h>

/* Environment variable for specifying the MSRs to use */
#define HB_ENERGY_MSR_ENV_VAR "HEARTBEAT_ENERGY_MSRS"
#define HB_ENERGY_MSRS_DELIMS ", :;|"

int hb_energy_init_msr(void);

double hb_energy_read_total_msr(int64_t last_hb_time, int64_t curr_hb_time);

int hb_energy_finish_msr(void);

char* hb_energy_get_source_msr(void);

hb_energy_impl* hb_energy_impl_alloc_msr(void);

#ifdef __cplusplus
}
#endif

#endif
