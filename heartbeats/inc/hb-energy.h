/**
 * Internal API for reading energy values. Different platforms may need
 * different implementations.  For example, X86-based systems may be able to
 * share an implementation that reads from the MSR, while other systems may
 * have to read from power sensors and convert power readings to energy.
 *
 * Pointers to implementations of the typedef'd functions below are included
 * in the hb_energy_impl struct for encapsulation.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _HB_ENERGY_H_
#define _HB_ENERGY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <time.h>

/**
 * Open required file(s), start necessary background tasks, etc.
 */
typedef int (*hb_energy_init_func) (void);

/**
 * Get the total energy used since init was called.
 */
typedef double (*hb_energy_read_total_func) (int64_t last_hb_time, int64_t curr_hb_time);

/**
 * Stop background tasks, close open file(s), etc.
 */
typedef int (*hb_energy_finish_func) (void);

/**
 * Get a human-readable description of the energy calculation source.
 */
typedef char* (*hb_energy_get_source_func) (void);

/**
 * A structure to encapsulate a complete implementation. Each field is a
 * pointer to a required function.
 */
typedef struct {
  hb_energy_init_func finit;
  hb_energy_read_total_func fread;
  hb_energy_finish_func ffinish;
  hb_energy_get_source_func fsource;
} hb_energy_impl;

/*
 * Compile with HB_ENERGY_IMPL when there is one implementation that is
 * accessed via interface functions rather than directly. If you enable this,
 * you are responsible for specifying the implementation during compilation.
 */
#ifdef HB_ENERGY_IMPL
int hb_energy_init(void);

double hb_energy_read_total(int64_t last_hb_time, int64_t curr_hb_time);

int hb_energy_finish(void);

char* hb_energy_get_source(void);

/**
 * Factory function to alloc a hb_energy_impl specific to this implementation.
 */
hb_energy_impl* hb_energy_impl_alloc(void);
#endif

/**
 * Convert a timespec struct into a nanosecond value.
 */
static inline int64_t to_nanosec(struct timespec* ts) {
  return (int64_t) ts->tv_sec * 1000000000 + (int64_t) ts->tv_nsec;
}

/**
 * Calculate the difference in seconds from two nanosecond values.
 */
static inline double diff_sec(int64_t ns_start, int64_t ns_end) {
  return (ns_end - ns_start ) / 1000000000.0;
}

#ifdef __cplusplus
}
#endif

#endif
