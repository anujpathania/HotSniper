/**
 * Energy reading for an ODROID-XU+E.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _HB_ENERGY_ODROIDXUE_H_
#define _HB_ENERGY_ODROIDXUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hb-energy.h"
#include <inttypes.h>

int hb_energy_init_odroidxue(void);

double hb_energy_read_total_odroidxue(int64_t last_hb_time, int64_t curr_hb_time);

int hb_energy_finish_odroidxue(void);

char* hb_energy_get_source_odroidxue(void);

hb_energy_impl* hb_energy_impl_alloc_odroidxue(void);

#ifdef __cplusplus
}
#endif

#endif
