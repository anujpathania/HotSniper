/**
* Energy reading from a Watts up? PRO
*
* @author Connor Imes
* @date 2014-06-30
*/

#include "hb-energy.h"
#include "hb-energy-wattsup.h"
#include <wattsup/wattsup_common.h>
#include <wattsup/util.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>

// must wait at least one second between WattsUp polls
#define WU_MIN_INTERVAL_SEC 1.0
#define WU_MAX_RETRIES 10

int wu_fd;
int64_t wu_last_read_time;
double wu_last_pwr;
double wu_total_energy;

#ifdef HB_ENERGY_IMPL
int hb_energy_init(void) {
  return hb_energy_init_wattsup();
}

double hb_energy_read_total(int64_t last_hb_time, int64_t curr_hb_time) {
  return hb_energy_read_total_wattsup(last_hb_time, curr_hb_time);
}

int hb_energy_finish(void) {
  return hb_energy_finish_wattsup();
}

char* hb_energy_get_source(void) {
  return hb_energy_get_source_wattsup();
}

hb_energy_impl* hb_energy_impl_alloc(void) {
  return hb_energy_impl_alloc_wattsup();
}
#endif

int hb_energy_init_wattsup(void) {
  int ret = 0;

  char* wu_filename = getenv(ENV_WU_DEV_FILE);
  if (wu_filename == NULL) {
    wu_filename = WU_DEV_FILE_DEFAULT;
  }
  // printf("Looking for WattsUp at %s.\n", wu_filename);

  enable_field("watts");
  ret += open_device(wu_filename, &wu_fd);
  if (ret) {
    fprintf(stderr, "Failed to open WattsUp device at %s\n", wu_filename);
    hb_energy_finish_wattsup();
    return ret;
  }
  ret = setup_serial_device(wu_fd);
  if (ret) {
    perror("wattsup:setup");
    fprintf(stderr, "Trying to setup %s\n", wu_filename);
    hb_energy_finish_wattsup();
    return ret;
  }
  ret = wu_clear(wu_fd);
  if (ret) {
    perror("wattsup:clear");
    fprintf(stderr, "Trying to clear %s\n", wu_filename);
    hb_energy_finish_wattsup();
    return ret;
  }

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  wu_last_read_time = to_nanosec(&now); // set current time but don't actually read
  wu_last_pwr = 0;
  wu_total_energy = 0;

  return ret;
}

int hb_energy_finish_wattsup(void) {
  int ret = 0;
  if (wu_fd > 0) {
    ret += close(wu_fd);
  }
  return ret;
}

/**
 * Read power from WattsUp and estimate energy usage. If called more frequently than
 * once per second, the previous power value will be used. This function works OK
 * assuming the power doesn't fluctuate much between calls since there is no direct
 * energy measurement.
 */
double hb_energy_read_total_wattsup(int64_t last_hb_time, int64_t curr_hb_time) {
  double data = wu_last_pwr; // default to last power value
  last_hb_time = last_hb_time < 0 ? wu_last_read_time : last_hb_time;
  if (diff_sec(wu_last_read_time, curr_hb_time) > WU_MIN_INTERVAL_SEC) {
    struct wu_packet p = {
      .label = {
        [0] = "watts",
        [1] = "volts",
        [2] = "amps",
        [3] = "watt hours",
        [4] = "cost",
        [5] = "mo. kWh",
        [6] = "mo. cost",
        [7] = "max watts",
        [8] = "max volts",
        [9] = "max amps",
        [10] = "min watts",
        [11] = "min volts",
        [12] = "min amps",
        [13] = "power factor",
        [14] = "duty cycle",
        [15] = "power cycle",
        [16] = "frequency",
        [17] = "VA"
      },
    };
    int retry = 0;
    int ret;
    while (1) {
      ret = wu_read(wu_fd, &p);
      if (ret && retry++ <= WU_MAX_RETRIES) {
        // fprintf(stderr, "Bad WattsUp record back, retrying\n");
        continue;
      } else if (ret) {
        fprintf(stderr, "Failed to read WattsUp - using last power value: %f\n", data);
      } else {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        wu_last_read_time = to_nanosec(&now); // update timestamp to time after read
        data = strtod(p.field[wu_field_watts], NULL) / 10.0;
      }
      break;
    }
  }

  // convert from power to energy using timestamps
  double joules = data * diff_sec(last_hb_time, curr_hb_time);
  wu_total_energy += joules;
  wu_last_pwr = data; // doesn't change if WattsUp wasn't read

  return wu_total_energy;
}

char* hb_energy_get_source_wattsup(void) {
  return "Watts up? PRO";
}

hb_energy_impl* hb_energy_impl_alloc_wattsup(void) {
  hb_energy_impl* hei = (hb_energy_impl*) malloc(sizeof(hb_energy_impl));
  hei->finit = &hb_energy_init_wattsup;
  hei->fread = &hb_energy_read_total_wattsup;
  hei->ffinish = &hb_energy_finish_wattsup;
  hei->fsource = &hb_energy_get_source_wattsup;
  return hei;
}
