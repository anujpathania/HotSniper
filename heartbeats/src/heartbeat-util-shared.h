#ifndef _HEARTBEAT_UTIL_SHARED_H_
#define _HEARTBEAT_UTIL_SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/* Determine which heartbeat implementation to use */
#if defined(HEARTBEAT_MODE_ACC_POW)
#include "heartbeat-accuracy-power.h"
#include "heartbeat-accuracy-power-types.h"
#elif defined(HEARTBEAT_MODE_ACC)
#include "heartbeat-accuracy.h"
#include "heartbeat-accuracy-types.h"
#else
#include "heartbeat.h"
#include "heartbeat-types.h"
#endif

_HB_global_state_t* HB_alloc_state(int pid);

_heartbeat_record_t* HB_alloc_log(int pid, int64_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
