/**
 * Energy reading for an ODROID-XU+E.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */

#include "hb-energy.h"
#include "hb-energy-odroidxue.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>

// sensor files
#define ODROID_PWR_FILENAME_TEMPLATE "/sys/bus/i2c/drivers/INA231/%s/sensor_W"
#define ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE "/sys/bus/i2c/drivers/INA231/%s/enable"
#define ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE "/sys/bus/i2c/drivers/INA231/%s/update_period"
#define ODROID_A15_DIR "4-0040"
#define ODROID_A7_DIR "4-0045"
#define ODROID_MEM_DIR "4-0041"
#define ODROID_GPU_DIR "4-0044"

// sensor update interval in microseconds
#define ODROID_SENSOR_READ_DELAY_US_DEFAULT 263808
static int odroid_read_delay_us;

// sensor file descriptors
static int odroid_pwr_a15;
static int odroid_pwr_a7;
static int odroid_pwr_mem;
static int odroid_pwr_gpu;

static int64_t odroid_start_time;
static double odroid_total_energy;

// thread variables
static double odroid_hb_pwr_avg;
static double odroid_hb_pwr_avg_last;
static int odroid_hb_pwr_avg_count;
static pthread_mutex_t odroid_sensor_mutex;
static pthread_t odroid_sensor_thread;
static int odroid_read_sensors;

#ifdef HB_ENERGY_IMPL
int hb_energy_init(void) {
  return hb_energy_init_odroidxue();
}

double hb_energy_read_total(int64_t last_hb_time, int64_t curr_hb_time) {
  return hb_energy_read_total_odroidxue(last_hb_time, curr_hb_time);
}

int hb_energy_finish(void) {
  return hb_energy_finish_odroidxue();
}

char* hb_energy_get_source(void) {
  return hb_energy_get_source_odroidxue();
}

hb_energy_impl* hb_energy_impl_alloc(void) {
  return hb_energy_impl_alloc_odroidxue();
}
#endif

static inline int odroid_open_file(char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("odroid_open_file");
    fprintf(stderr, "Trying to open %s\n", filename);
  }
  return fd;
}

static inline long get_sensor_update_interval(char* sensor) {
  char ui_filename[BUFSIZ];
  int fd;
  char cdata[BUFSIZ];
  long val;
  int read_ret;

  sprintf(ui_filename, ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE, sensor);
  fd = odroid_open_file(ui_filename);
  if (fd < 0) {
    return -1;
  }
  read_ret = read(fd, cdata, sizeof(val));
  close(fd);
  if (read_ret < 0) {
    return -1;
  }
  val = atol(cdata);
  return val;
}

/**
 * Return 0 if the sensor is enabled, 1 if not, -1 on error.
 */
static inline int odroid_check_sensor_enabled(char* sensor) {
  char enable_filename[BUFSIZ];
  int fd;
  char cdata[BUFSIZ];
  int val;
  int read_ret;

  sprintf(enable_filename, ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE, sensor);
  fd = odroid_open_file(enable_filename);
  if (fd < 0) {
    return -1;
  }
  read_ret = read(fd, cdata, sizeof(val));
  close(fd);
  if (read_ret < 0) {
    return -1;
  }
  val = atoi(cdata);
  return val == 0 ? 1 : 0;
}

static inline long get_update_interval() {
  long ret = 0;
  long tmp;
  tmp = get_sensor_update_interval(ODROID_A15_DIR);
  if (tmp < 0) {
    fprintf(stderr, "get_update_interval: Warning: could not read A15 update "
            "interval\n");
  }
  ret = tmp > ret ? tmp : ret;
  tmp = get_sensor_update_interval(ODROID_A7_DIR);
  if (tmp < 0) {
    fprintf(stderr, "get_update_interval: Warning: could not read A7 update "
            "interval\n");
  }
  ret = tmp > ret ? tmp : ret;
  tmp = get_sensor_update_interval(ODROID_MEM_DIR);
  if (tmp < 0) {
    fprintf(stderr, "get_update_interval: Warning: could not read MEM update "
            "interval\n");
  }
  ret = tmp > ret ? tmp : ret;
  tmp = get_sensor_update_interval(ODROID_GPU_DIR);
  if (tmp < 0) {
    fprintf(stderr, "get_update_interval: Warning: could not read GPU update "
            "interval\n");
  }
  ret = tmp > ret ? tmp : ret;
  if (ret == 0) {
    // failed to read values - use default
    ret = ODROID_SENSOR_READ_DELAY_US_DEFAULT;
    fprintf(stderr, "get_update_interval: Using default value: %ld us\n", ret);
  }
  return ret;
}

/**
 * Stop the sensors polling pthread, cleanup, and close sensor files.
 */
int hb_energy_finish_odroidxue(void) {
  int ret = 0;
  // stop sensors polling thread and cleanup
  odroid_read_sensors = 0;
  if(pthread_join(odroid_sensor_thread, NULL)) {
    fprintf(stderr, "Error joining ODROID sensor polling thread.\n");
    ret -= 1;
  }
  if(pthread_mutex_destroy(&odroid_sensor_mutex)) {
    fprintf(stderr, "Error destroying pthread mutex for ODROID sensor polling thread.\n");
    ret -= 1;
  }
  // close individual sensor files
  if (odroid_pwr_a15 > 0) {
    ret += close(odroid_pwr_a15);
  }
  if (odroid_pwr_a15 > 0) {
    ret += close(odroid_pwr_a7);
  }
  if (odroid_pwr_a15 > 0) {
    ret += close(odroid_pwr_mem);
  }
  if (odroid_pwr_a15 > 0) {
    ret += close(odroid_pwr_gpu);
  }
  return ret;
}

/**
 * Read power from a sensor file
 */
static inline double odroid_read_pwr(int fd) {
  double val;
  char cdata[sizeof(double)];
  int data_size = pread(fd, cdata, sizeof(double), 0);
  if (data_size != sizeof(double)) {
    perror("odroid_read_pwr");
    return -1;
  }
  val = strtod(cdata, NULL);
  return val;
}

/**
 * pthread function to poll the sensors at regular intervals and
 * keep a running average of power between heartbeats.
 */
void* odroid_poll_sensors(void* args) {
  double pwr_a15, pwr_a7, pwr_mem, pwr_gpu, sum;
  struct timespec ts_interval;
  ts_interval.tv_sec = odroid_read_delay_us / (1000 * 1000);
  ts_interval.tv_nsec = (odroid_read_delay_us % (1000 * 1000) * 1000);
  while(odroid_read_sensors > 0) {
    // read individual sensors
    pwr_a15 = odroid_read_pwr(odroid_pwr_a15);
    pwr_a7 = odroid_read_pwr(odroid_pwr_a7);
    pwr_mem = odroid_read_pwr(odroid_pwr_mem);
    pwr_gpu = odroid_read_pwr(odroid_pwr_gpu);
    // sum the power values
    if (pwr_a15 < 0 || pwr_a7 < 0 || pwr_mem < 0 || pwr_gpu < 0) {
      fprintf(stderr, "At least one ODROID power sensor returned bad value - skipping this reading\n");
    } else {
      sum = pwr_a15 + pwr_a7 + pwr_mem + pwr_gpu;
      // keep running average between heartbeats
      pthread_mutex_lock(&odroid_sensor_mutex);
      odroid_hb_pwr_avg = (sum + odroid_hb_pwr_avg_count * odroid_hb_pwr_avg) / (odroid_hb_pwr_avg_count + 1);
      odroid_hb_pwr_avg_count++;
      pthread_mutex_unlock(&odroid_sensor_mutex);
    }
    // sleep for the update interval of the sensors
    // TODO: Should use a conditional here so thread can be woken up to end immediately
    nanosleep(&ts_interval, NULL);
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int hb_energy_init_odroidxue(void) {
  int ret = 0;
  char odroid_pwr_filename[BUFSIZ];

  // reset the total energy reading
  odroid_total_energy = 0;

  // ensure that the sensors are enabled
  if (odroid_check_sensor_enabled(ODROID_A15_DIR)) {
    fprintf(stderr, "hb_energy_init: A15 power sensor not enabled\n");
    return -1;
  }
  if (odroid_check_sensor_enabled(ODROID_A7_DIR)) {
    fprintf(stderr, "hb_energy_init: A7 power sensor not enabled\n");
    return -1;
  }
  if (odroid_check_sensor_enabled(ODROID_MEM_DIR)) {
    fprintf(stderr, "hb_energy_init: MEM power sensor not enabled\n");
    return -1;
  }
  if (odroid_check_sensor_enabled(ODROID_GPU_DIR)) {
    fprintf(stderr, "hb_energy_init: GPU power sensor not enabled\n");
    return -1;
  }

  // open individual sensor files
  sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, ODROID_A15_DIR);
  odroid_pwr_a15 = odroid_open_file(odroid_pwr_filename);
  if (odroid_pwr_a15 < 0) {
    hb_energy_finish_odroidxue();
    return -1;
  }
  sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, ODROID_A7_DIR);
  odroid_pwr_a7 = odroid_open_file(odroid_pwr_filename);
  if (odroid_pwr_a7 < 0) {
    hb_energy_finish_odroidxue();
    return -1;
  }
  sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, ODROID_MEM_DIR);
  odroid_pwr_mem = odroid_open_file(odroid_pwr_filename);
  if (odroid_pwr_mem < 0) {
    hb_energy_finish_odroidxue();
    return -1;
  }
  sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, ODROID_GPU_DIR);
  odroid_pwr_gpu = odroid_open_file(odroid_pwr_filename);
  if (odroid_pwr_gpu < 0) {
    hb_energy_finish_odroidxue();
    return -1;
  }

  // get the delay time between reads
  odroid_read_delay_us = get_update_interval();

  // track start time
  struct timespec now;
  clock_gettime( CLOCK_REALTIME, &now );
  odroid_start_time = to_nanosec(&now);

  // start sensors polling thread
  odroid_read_sensors = 1;
  odroid_hb_pwr_avg = 0;
  odroid_hb_pwr_avg_last = 0;
  odroid_hb_pwr_avg_count = 0;
  ret = pthread_mutex_init(&odroid_sensor_mutex, NULL);
  if(ret) {
    fprintf(stderr, "Failed to create ODROID sensors mutex.\n");
    hb_energy_finish_odroidxue();
    return ret;
  }
  ret = pthread_create(&odroid_sensor_thread, NULL, odroid_poll_sensors, NULL);
  if (ret) {
    fprintf(stderr, "Failed to start ODROID sensors thread.\n");
    hb_energy_finish_odroidxue();
    return ret;
  }

  return ret;
}

/**
 * Estimate energy from the average power since last heartbeat.
 */
double hb_energy_read_total_odroidxue(int64_t last_hb_time, int64_t curr_hb_time) {
  double result;
  last_hb_time = last_hb_time < 0 ? odroid_start_time : last_hb_time;
  // it's also assumed that curr_hb_time >= last_hb_time

  pthread_mutex_lock(&odroid_sensor_mutex);
  // convert from power to energy using timestamps
  odroid_hb_pwr_avg_last = odroid_hb_pwr_avg > 0 ? odroid_hb_pwr_avg : odroid_hb_pwr_avg_last;
  odroid_total_energy += odroid_hb_pwr_avg_last * diff_sec(last_hb_time, curr_hb_time);
  result = odroid_total_energy;
  // reset running power average
  odroid_hb_pwr_avg = 0;
  odroid_hb_pwr_avg_count = 0;
  pthread_mutex_unlock(&odroid_sensor_mutex);

  return result;
}

char* hb_energy_get_source_odroidxue(void) {
  return "ODROID-XU+E Power Sensors (A15, A7, MEM, GPU)";
}

hb_energy_impl* hb_energy_impl_alloc_odroidxue(void) {
  hb_energy_impl* hei = (hb_energy_impl*) malloc(sizeof(hb_energy_impl));
  hei->finit = &hb_energy_init_odroidxue;
  hei->fread = &hb_energy_read_total_odroidxue;
  hei->ffinish = &hb_energy_finish_odroidxue;
  hei->fsource = &hb_energy_get_source_odroidxue;
  return hei;
}
