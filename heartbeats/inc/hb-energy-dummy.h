/**
 * Dummy hb-energy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */
#ifndef _HB_ENERGY_DUMMY_H_
#define _HB_ENERGY_DUMMY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hb-energy.h"
#include <inttypes.h>

int hb_energy_init_dummy(void);

double hb_energy_read_total_dummy(int64_t last_hb_time, int64_t curr_hb_time);

int hb_energy_finish_dummy(void);

char* hb_energy_get_source_dummy(void);

hb_energy_impl* hb_energy_impl_alloc_dummy(void);

#ifdef __cplusplus
}
#endif

#endif
