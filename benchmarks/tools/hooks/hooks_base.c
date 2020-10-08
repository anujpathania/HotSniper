#include <stdio.h>

#include "sim_api.h"
#include "hooks_base.h"

void parmacs_roi_begin() {
  printf("[HOOKS] Entering ROI\n"); fflush(NULL);
  SimRoiStart();
}

void parmacs_roi_end() {
  SimRoiEnd();
  printf("[HOOKS] Leaving ROI\n"); fflush(NULL);
}

void parmacs_iter_begin(int iter)
{
}

void parmacs_iter_end(int iter)
{
}

// Fortran linkage
void parmacs_roi_begin_() { parmacs_roi_begin(); }
void parmacs_roi_end_() { parmacs_roi_end(); }
void parmacs_iter_begin_(int iter) { parmacs_iter_begin(iter); }
void parmacs_iter_end_(int iter) { parmacs_iter_end(iter); }

void parmacs_setup(void) __attribute ((constructor));
void parmacs_setup(void) {
#if defined(PARMACS_NO_ROI)
  parmacs_roi_begin();
#endif
}

void parmacs_shutdown(void) __attribute ((destructor));
void parmacs_shutdown(void) {
#if defined(PARMACS_NO_ROI)
  parmacs_roi_end();
#endif
}
