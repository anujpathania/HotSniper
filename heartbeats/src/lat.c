/** \file
 *  \brief Example: Latency
 *  \author Hank Hoffmann
 *  \version 1.0
 *  \example lat.c
 *  Latency Example
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "heartbeat.h"
#include "heartbeat-types.h"
#include "heart_rate_monitor.h"

heartbeat_t* heart;
heart_rate_monitor_t hrm;

/**
       *
       * @param apps
       * @return count integer: number of heartbeats
       */
int get_heartbeat_apps(int* apps) {

   pid_t pid;
   int rv;
   int	commpipe[2];		/* This holds the fd for the input & output of the pipe */
   char string[1024][100];
   int count = 0;

   /* Setup communication pipeline first */
   if(pipe(commpipe)){
      fprintf(stderr,"Pipe error!\n");
      exit(1);
   }

   /* Attempt to fork and check for errors */
   if( (pid=fork()) < 0 ){
      fprintf(stderr,"Fork error. Exiting.\n");  /* something went wrong */
      exit(1);
   }

   if(pid){
      /* A positive (non-negative) PID indicates the parent process */
      dup2(commpipe[0],0);
      close(commpipe[1]);
      while(fgets(string[count], 100, stdin)) {
         apps[count] = atoi(string[count]);
         count++;
      }

      //printf("From the system: found %d apps\n", count);
      //printf("From the system: app is %d\n", apps[0]);
      //wait(&rv);				/* Wait for child process to end */
      wait(&rv);
      //fprintf(stderr,"Child exited with a %d value\n",rv);
   }
   else{
      /* A zero PID indicates that this is the child process */
      dup2(commpipe[1],1);	/* Replace stdout with the out side of the pipe */
      close(commpipe[0]);		/* Close unused side of pipe (in side) */
      /* Replace the child fork with a new process */
      //FIXME
      //if(execl("/bin/ls","/bin/ls","/scratch/etc/heartbeat/heartbeat-enabled-apps/",(char*) NULL) == -1){
      if(execl("/bin/ls","/bin/ls",getenv("HEARTBEAT_ENABLED_DIR"),(char*) NULL) == -1){
         fprintf(stderr,"execl Error!");
         exit(1);
      }
   }

   close(commpipe[0]);
   return count;
}

/**
       *
       * @param logname char pointer
       * @param iters integer
       */
void app(char* logname, int iters)
{
  heartbeat_record_t record;
   // init heartbeats and a monitor
   //printf("executing the app code\n");
   heart = heartbeat_init(10, 100, NULL, 0, 1000000);
   int apps[2];
   while( get_heartbeat_apps(apps) != 2 );
   int pid = getpid();
   if ( apps[0] == pid )
      heart_rate_monitor_init(&hrm, apps[1]);
   else
      heart_rate_monitor_init(&hrm, apps[0]);

   // quiesce for a bit then synchronize
   usleep(1000000);

   heartbeat(heart, -2);

   int tag;
   do
   {
      int rc = -1;
      while (rc != 0)
       rc = hrm_get_current(&hrm, &record);
      tag = record.tag;
   } while( tag != -1 );


   // no timer code needed here
   int i;
   for(i = iters; i > 0; i--) {

      // issue a heartbeat
     heartbeat(heart, i);

     // wait for a heartbeat
     do
       {
	 int rc = -1;
	 while (rc != 0)
	   rc = hrm_get_current(&hrm, &record);
	 tag = record.tag;

       } while( tag != (iters - i) );
   }
      heartbeat_finish(heart);

   // no timer code needed here
}

/**
       *
       * @param logname char pointer
       * @param iters integer
       */
void sys(char* logname, int iters)
{
  heartbeat_record_t record;
   // init heartbeats and a monitor
  heart = heartbeat_init(10, 100, NULL, 0, 1000000);
  int apps[2];
  while( get_heartbeat_apps(apps) != 2 );
  int pid = getpid();
  if ( apps[0] == pid )
    heart_rate_monitor_init(&hrm, apps[1]);
  else
    heart_rate_monitor_init(&hrm, apps[0]);

  // quiesce for a bit then synchronize
  usleep(1000000);
  int tag;
  do
    {
      int rc = -1;
      while (rc != 0)
	rc = hrm_get_current(&hrm, &record);
      tag = record.tag;
    } while( tag != -2 );
  heartbeat(heart, -1);

  // start timer
  struct timespec time_info;
  int64_t time1, time2;
   clock_gettime( CLOCK_REALTIME, &time_info );
   time1 = ( (int64_t) time_info.tv_sec * 1000000000 + (int64_t) time_info.tv_nsec );

   int i;
   for(i = 0; i < iters; i++) {

     // wait for a heartbeat
     do
       {
	 int rc = -1;
	 while (rc != 0)
	   rc = hrm_get_current(&hrm, &record);
	 tag = record.tag;
       } while( tag != (iters - i) );

     // issue a heartbeat
     heartbeat(heart, i);
   }

   // end timer
   clock_gettime( CLOCK_REALTIME, &time_info );
   time2 = ( (int64_t) time_info.tv_sec * 1000000000 + (int64_t) time_info.tv_nsec );

   // latency calculation in units of ns
   double latency = ((double) (time2 - time1)) / ((double) (iters * 2)) ;
   printf("average heartbeats latency: %0.2f ns\n", latency);
   printf("  = %.0f cycles @ 3.16 GHz\n", 3.16 * latency );
   heartbeat_finish(heart);

}

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

   // fork a process.
   // this process models the application (and does cleanup)
   // the child process models the system

   int rv;
   pid_t pid = fork();

   if( pid < 0 ){
      fprintf(stderr,"Fork error. Exiting.\n");  /* something went wrong */
      exit(1);
   }

   if ( pid == 0 )
   {
      sys( argv[2], atoi(argv[1]) );
      return 0;
   }
   else
   {
      app( argv[2], atoi(argv[1]) );
      wait(&rv);

      // cleanup
      return 0;
   }
   return 0;
}
