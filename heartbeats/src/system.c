/** \file
 *  \brief Example: Something more interesting
 *  \author Hank Hoffmann
 *  \version 1.0
 *  \example system.c
 *  A More Interesting Example
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heart_rate_monitor.h"
#include "heartbeat-types.h"
#include <assert.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>

heart_rate_monitor_t heart;

typedef struct {
  int tag;
  double rate;
} heart_data_t;



//static int pipe_set_up = 0;

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
  if( (pid=fork()) == -1){
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
    wait(&rv);				/* Wait for child process to end */
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
       */
int main(int argc, char** argv) {
  int n = 0;
  int i;
  const int MAX = atoi(argv[1]);

  int apps[1024];

   if(getenv("HEARTBEAT_ENABLED_DIR") == NULL) {
     fprintf(stderr, "ERROR: need to define environment variable HEARTBEAT_ENABLED_DIR (see README)\n");
     return 1;
   }

  heart_data_t* records = (heart_data_t*) malloc(MAX*sizeof(heart_data_t));
  int last_tag = -1;

  while(n == 0) {
    n = get_heartbeat_apps(apps);
  }

  //printf("apps[0] = %d\n", apps[0]);

  // For this test we only allow one heartbeat enabled app
  assert(n==1);

#if 1
  int rc = heart_rate_monitor_init(&heart, apps[0]);

  if (rc != 0)
    printf("Error attaching memory\n");

  printf("buffer depth is %lld\n", (long long int) heart.state->buffer_depth);

  i = 0;
  int current_tag = -1;
  while(last_tag < MAX-1) {
    heartbeat_record_t record;

    while(current_tag == last_tag) {
      int rc = -1;
      while (rc != 0)
	rc = hrm_get_current(&heart, &record);
      current_tag = record.tag;
    }
    records[i].tag = last_tag = current_tag;
    records[i].rate = record.window_rate;
    //printf("System: %d  %d\n", current_tag, last_tag);
    //    printf("System: %d, %f\n", records[i].tag, records[i].rate);
    i++;
  }

  //printf("System: Global heart rate: %f, Current heart rate: %f\n", heart.global_heartrate, heart.window_heartrate);

  for(i = 0; i < MAX; i++) {
    printf("%d, %f\n", records[i].tag, records[i].rate);
  }
  heart_rate_monitor_finish(&heart);
#endif

  return 0;
}
