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

#include "hb-energy.h"
#include "hb-energy-msr.h"
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define MSR_RAPL_POWER_UNIT		0x606

/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT	0x610
#define MSR_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS		0x613
#define MSR_PKG_POWER_INFO		0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT		0x638
#define MSR_PP0_ENERGY_STATUS		0x639
#define MSR_PP0_POLICY			0x63A
#define MSR_PP0_PERF_STATUS		0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT		0x640
#define MSR_PP1_ENERGY_STATUS		0x641
#define MSR_PP1_POLICY			0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT		0x618
#define MSR_DRAM_ENERGY_STATUS		0x619
#define MSR_DRAM_PERF_STATUS		0x61B
#define MSR_DRAM_POWER_INFO		0x61C

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET	0
#define POWER_UNIT_MASK		0x0F

#define ENERGY_UNIT_OFFSET	0x08
#define ENERGY_UNIT_MASK	0x1F00

#define TIME_UNIT_OFFSET	0x10
#define TIME_UNIT_MASK		0xF000

/* Shared variables */
int msr_count = 0;
int* msr_fds = NULL;
double* msr_energy_units = NULL;

#ifdef HB_ENERGY_IMPL
int hb_energy_init(void) {
  return hb_energy_init_msr();
}

double hb_energy_read_total(int64_t last_hb_time, int64_t curr_hb_time) {
  return hb_energy_read_total_msr(last_hb_time, curr_hb_time);
}

int hb_energy_finish(void) {
  return hb_energy_finish_msr();
}

char* hb_energy_get_source(void) {
  return hb_energy_get_source_msr();
}

hb_energy_impl* hb_energy_impl_alloc(void) {
  return hb_energy_impl_alloc_msr();
}
#endif

static inline int open_msr(int core) {
  char msr_filename[BUFSIZ];
  int fd;

  sprintf(msr_filename, "/dev/cpu/%d/msr", core);
  fd = open(msr_filename, O_RDONLY);
  if ( fd < 0 ) {
    if ( errno == ENXIO ) {
      fprintf(stderr, "open_msr: No CPU %d\n", core);
    } else if ( errno == EIO ) {
      fprintf(stderr, "open_msr: CPU %d doesn't support MSRs\n", core);
    } else {
      perror("open_msr:open");
      fprintf(stderr, "Trying to open %s\n", msr_filename);
    }
  }
  //printf("fd = %d\n", fd);
  return fd;
}

static inline long long read_msr(int fd, int which) {
  uint64_t data = 0;
  uint64_t data_size = pread(fd, &data, sizeof data, which);

  //prinuse-armtf("Data size = %lld, fd = %d\n", data_size,fd);

  if ( data_size != sizeof data ) {
    perror("read_msr:pread");
    return -1;
  }

  return (long long)data;
}

int hb_energy_init_msr(void) {
  int ncores = 0;
  int i;
  long long power_unit_data_ll;
  double power_unit_data;
  // get a delimited list of cores with MSRs to read from
  char* env_cores = getenv(HB_ENERGY_MSR_ENV_VAR);
  char* env_cores_tmp; // need a writable string for strtok function
  if (env_cores == NULL) {
    // default to using core 0
    env_cores = "0";
  }

  // first determine the number of MSRs to be accessed
  env_cores_tmp = strdup(env_cores);
  char* tok = strtok(env_cores_tmp, HB_ENERGY_MSRS_DELIMS);
  while (tok != NULL) {
    ncores++;
    tok = strtok(NULL, HB_ENERGY_MSRS_DELIMS);
  }
  free(env_cores_tmp);
  if (ncores == 0) {
    fprintf(stderr, "hb_energy_init: Failed to parse core numbers from "
            "%s environment variable.\n", HB_ENERGY_MSR_ENV_VAR);
    return -1;
  }

  // Now determine which cores' MSRs will be accessed
  int core_ids[ncores];
  env_cores_tmp = strdup(env_cores);
  tok = strtok(env_cores_tmp, HB_ENERGY_MSRS_DELIMS);
  for (i = 0; tok != NULL; tok = strtok(NULL, HB_ENERGY_MSRS_DELIMS), i++) {
    core_ids[i] = atoi(tok);
    // printf("hb_energy_init: using MSR for core %d\n", core_ids[i]);
  }
  free(env_cores_tmp);

  // allocate shared variables
  msr_fds = (int*) malloc(ncores * sizeof(int));
  msr_energy_units = (double*) malloc(ncores * sizeof(double));

  // open the MSR files
  for (i = 0; i < ncores; i++) {
    msr_fds[i] = open_msr(core_ids[i]);
    if (msr_fds[i] < 0) {
      hb_energy_finish_msr();
      return -1;
    }
    power_unit_data_ll = read_msr(msr_fds[i], MSR_RAPL_POWER_UNIT);
    if (power_unit_data_ll < 0) {
      hb_energy_finish_msr();
      return -1;
    }
    power_unit_data = (double) ((power_unit_data_ll >> 8) & 0x1f);
    msr_energy_units[i] = pow(0.5, power_unit_data);
  }

  msr_count = ncores;
  return 0;
}

double hb_energy_read_total_msr(int64_t last_hb_time, int64_t curr_hb_time) {
  int i;
  double msr_val;
  double total = 0.0;
  for (i = 0; i < msr_count; i++) {
    msr_val = (double) read_msr(msr_fds[i], MSR_PKG_ENERGY_STATUS);
    if (msr_val < 0.0) {
      fprintf(stderr, "hb_energy_read_total: got bad energy value from MSR\n");
      return -1.0;
    }
    total += msr_val * msr_energy_units[i];
  }
  return total;
}

int hb_energy_finish_msr(void) {
  int ret = 0;
  if (msr_fds != NULL) {
    int i;
    for (i = 0; i < msr_count; i++) {
      if (msr_fds[i] > 0) {
        ret += close(msr_fds[i]);
      }
    }
  }
  free(msr_fds);
  msr_fds = NULL;
  free(msr_energy_units);
  msr_energy_units = NULL;
  msr_count = 0;
  return ret;
}

char* hb_energy_get_source_msr(void) {
  return "X86 MSR";
}

hb_energy_impl* hb_energy_impl_alloc_msr(void) {
  hb_energy_impl* hei = (hb_energy_impl*) malloc(sizeof(hb_energy_impl));
  hei->finit = &hb_energy_init_msr;
  hei->fread = &hb_energy_read_total_msr;
  hei->ffinish = &hb_energy_finish_msr;
  hei->fsource = &hb_energy_get_source_msr;
  return hei;
}
