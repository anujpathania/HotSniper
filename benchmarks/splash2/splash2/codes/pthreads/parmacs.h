#ifndef PARMACS_H
#define PARMACS_H

#include <hooks_base.h>
#include "magic.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#define PARMACS_MAX_THREADS 1024
extern int ParmacsThreadNum;
extern pthread_t ParmacsThreads[];

#endif // PARMACS_H
