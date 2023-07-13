/** \file
 *  \brief Example: Throughput
 *  \author Hank Hoffmann
 *  \version 1.0
 *  \example tp.c
 *  Throughput Example
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "heartbeat.h"

heartbeat_t* heart;

/**
       *
       * @param argv[1]: total number of heartbeats to issue
       * @param argv[2]: name of log file
       */
int main(int argc, char** argv) {

   if ( argc != 3 )
   {
      printf("usage:\n");
      printf("  application num_beats, log_file\n");
      return -1;
   }
   if(getenv("HEARTBEAT_ENABLED_DIR") == NULL) {
     fprintf(stderr, "ERROR: need to define environment variable HEARTBEAT_ENABLED_DIR (see README)\n");
     return 1;
   }

   int i;
   const int MAX = atoi(argv[1]);

   heart = heartbeat_init(100, 1000, NULL, 0, 1000000);

   usleep(1000);

   for(i = 0; i < MAX; i++) {
     heartbeat(heart, i);
   }

   printf("Global heart rate: %f, Current heart rate: %f\n",
   hb_get_global_rate(heart), hb_get_windowed_rate(heart));

   heartbeat_finish(heart);
   return 0;
}
