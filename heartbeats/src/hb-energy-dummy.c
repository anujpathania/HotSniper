/**
 * Dummy hb-energy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include "hb-energy.h"
#include "hb-energy-dummy.h"
#include <stdlib.h>
#include <inttypes.h>

#ifdef HB_ENERGY_IMPL
int hb_energy_init(void) {
  return hb_energy_init_dummy();
}

double hb_energy_read_total(int64_t last_hb_time, int64_t curr_hb_time) {
  return hb_energy_read_total_dummy(last_hb_time, curr_hb_time);
}

int hb_energy_finish(void) {
  return hb_energy_finish_dummy();
}

char* hb_energy_get_source(void) {
  return hb_energy_get_source_dummy();
}

hb_energy_impl* hb_energy_impl_alloc(void) {
  return hb_energy_impl_alloc_dummy();
}
#endif

int hb_energy_init_dummy(void) {
  return 0;
}

double hb_energy_read_total_dummy(int64_t last_hb_time, int64_t curr_hb_time) {
  return 0.0;
}

int hb_energy_finish_dummy(void) {
  return 0;
}

char* hb_energy_get_source_dummy(void) {
  return "Dummy Source";
}

hb_energy_impl* hb_energy_impl_alloc_dummy(void) {
  hb_energy_impl* hei = (hb_energy_impl*) malloc(sizeof(hb_energy_impl));
  hei->finit = &hb_energy_init_dummy;
  hei->fread = &hb_energy_read_total_dummy;
  hei->ffinish = &hb_energy_finish_dummy;
  hei->fsource = &hb_energy_get_source_dummy;
  return hei;
}
