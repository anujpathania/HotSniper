/**
* Energy reading from a Watts up? PRO.
*
* @author Connor Imes
* @date 2014-06-30
*/
#ifndef _HB_ENERGY_WATTSUP_H_
#define _HB_ENERGY_WATTSUP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hb-energy.h"
#include <inttypes.h>

// Environment variable for specifying the device file to read from
// must not include the preceding "/dev/"!
#define ENV_WU_DEV_FILE "WATTSUP_DEV_FILE"
#define WU_DEV_FILE_DEFAULT "ttyUSB0"

int hb_energy_init_wattsup(void);

double hb_energy_read_total_wattsup(int64_t last_hb_time, int64_t curr_hb_time);

int hb_energy_finish_wattsup(void);

char* hb_energy_get_source_wattsup(void);

hb_energy_impl* hb_energy_impl_alloc_wattsup(void);

#ifdef __cplusplus
}
#endif

#endif
