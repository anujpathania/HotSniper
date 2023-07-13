/**
 * Calculate idle power using the hb-energy API.
 *
 * @author Connor Imes
 * @date 2014-06-29
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "hb-energy.h"

#define DEFAULT_SLEEP_SEC 10

/**
 * The only argument allowed is an integer value for the number of seconds to
 * sleep for.
 */
int main(int argc, char** argv) {
  struct timespec ts_sleep;
  struct timespec time_info;
  int64_t time_start;
  int64_t time_end;
  double joules_start;
  double joules_end;
  double watts;

  if (argc > 1) {
    ts_sleep.tv_sec = atoi(argv[1]);
    if (ts_sleep.tv_sec <= 0) {
      fprintf(stderr,
              "Sleep time must be > 0 seconds. Defaulting to %d seconds.\n",
              DEFAULT_SLEEP_SEC);
      ts_sleep.tv_sec = DEFAULT_SLEEP_SEC;
    }
  } else {
    ts_sleep.tv_sec = DEFAULT_SLEEP_SEC;
  }
  ts_sleep.tv_nsec = 0;

  // initialize
  if (hb_energy_init()) {
    fprintf(stderr, "Could not initialize energy reading from: %s\n",
            hb_energy_get_source());
    return 1;
  }

  /*
   * Take a reading immediately in case the source doesn't start its energy
   * value at 0 and/or ignores start and end times.
   */
  clock_gettime(CLOCK_REALTIME, &time_info);
  time_start = to_nanosec(&time_info);
  clock_gettime(CLOCK_REALTIME, &time_info);
  time_end = to_nanosec(&time_info);
  joules_start = hb_energy_read_total(time_start, time_end);

  // reset start time for sleep
  clock_gettime(CLOCK_REALTIME, &time_info);
  time_start = to_nanosec(&time_info);

  // sleep
  if (nanosleep(&ts_sleep, NULL)) {
    /*
     * We could recover and sleep further, but extra work was done which could
     * affect the results
     */
    fprintf(stderr, "Sleep failed - aborting.\n");
    hb_energy_finish();
    return 1;
  }

  clock_gettime(CLOCK_REALTIME, &time_info);
  time_end = to_nanosec(&time_info);

  // get energy/power used during sleep
  joules_end = hb_energy_read_total(time_start, time_end);
  watts = (joules_end - joules_start) / (time_end - time_start) * 1000000000.0;
  printf("%f\n", watts);

  // cleanup
  hb_energy_finish();

  return 0;
}
