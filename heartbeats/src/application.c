/** \file
 *  \brief Example: Basic Application
 *  \author Hank Hoffmann
 *  \version 1.0
 *  \example application.c
 *  Basic Application Example
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "heartbeat.h"

heartbeat_t* heart;


/**
       * compile with #define NOSLEEP for none
       * @param argv[1]: total number of heartbeats to issue
       * @param argv[2]: name of log file
       * @param argv[3]: sleep interval between beats
       */
int main(int argc, char** argv) {

   if ( argc != 5 )
   {
      printf("usage:\n");
      printf("  application num_beats, log_file, sleep_interval, buffer_depth\n");
      return -1;
   }
   if(getenv("HEARTBEAT_ENABLED_DIR") == NULL) {
     fprintf(stderr, "ERROR: need to define environment variable HEARTBEAT_ENABLED_DIR (see README)\n");
     return 1;
   }

   int i;
   int sleep = atoi(argv[3]);
   const int MAX = atoi(argv[1]);
   const int BD = atoi(argv[4]);

   printf("Init\n");
   heart = heartbeat_init(100, BD, NULL, 0, 1000000);

   if(heart == NULL)
     printf("Error allocating heartbeat data\n");

   printf("Sleep\n");
   usleep(3000000);

   printf("Heartbeats\n");
   for(i = 0; i < MAX; i++) {
     printf("Heartbeat %d\n", i);
     heartbeat(heart, i);
     usleep(sleep);
   }
   printf("Heartbeats Done\n");
      usleep(sleep);
      usleep(sleep);

   printf("Global heart rate: %f, Current heart rate: %f\n",
   hb_get_global_rate(heart), hb_get_windowed_rate(heart));

   heartbeat_finish(heart);
   return 0;
}
