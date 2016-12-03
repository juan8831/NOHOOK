//
//  main.c
//  PerformanceTest
//
//  Created by Juan Lopez on 11/2/16.
//  Copyright © 2016 Juan Lopez. All rights reserved.
//

/*
 test read, open, close, write,
 clone, exit, remove
 
 */

#include <pthread.h>
#include <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include<mach/mach.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <mach/thread_act.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <mach/task.h>

 #include <sys/resource.h>


void fileOperations();

FILE * fp;
FILE *ptr;
char name[FILENAME_MAX];


int getVirtualMemory();
int getPhysicalMemory();

int results();


typedef struct {
   double utime, stime;
} CPUTime;

int get_cpu_time(CPUTime *rpd, int thread_only)
{
   task_t task;
   kern_return_t error;
   mach_msg_type_number_t count;
   thread_array_t thread_table;
   thread_basic_info_t thi;
   thread_basic_info_data_t thi_data;
   unsigned table_size;
   struct task_basic_info ti;
   
   if (thread_only) {
      // just get time of this thread
      count = THREAD_BASIC_INFO_COUNT;
      thi = &thi_data;
      error = thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)thi, &count);
      rpd->utime = thi->user_time.seconds + thi->user_time.microseconds * 1e-6;
      rpd->stime = thi->system_time.seconds + thi->system_time.microseconds * 1e-6;
      return 0;
   }
   
   
   // get total time of the current process
   
   task = mach_task_self();
   count = TASK_BASIC_INFO_COUNT;
   error = task_info(task, TASK_BASIC_INFO, (task_info_t)&ti, &count);
   { /* calculate CPU times, adapted from top/libtop.c */
      unsigned i;
      // the following times are for threads which have already terminated and gone away
      rpd->utime = ti.user_time.seconds + ti.user_time.microseconds * 1e-6;
      rpd->stime = ti.system_time.seconds + ti.system_time.microseconds * 1e-6;
      error = task_threads(task, &thread_table, &table_size);
         thi = &thi_data;
      // for each active thread, add up thread time
      for (i = 0; i != table_size; ++i) {
         count = THREAD_BASIC_INFO_COUNT;
         error = thread_info(thread_table[i], THREAD_BASIC_INFO, (thread_info_t)thi, &count);
        
         if ((thi->flags & TH_FLAGS_IDLE) == 0) {
            rpd->utime += thi->user_time.seconds + thi->user_time.microseconds * 1e-6;
            rpd->stime += thi->system_time.seconds + thi->system_time.microseconds * 1e-6;
         }
         error = mach_port_deallocate(mach_task_self(), thread_table[i]);
         
      }
      error = vm_deallocate(mach_task_self(), (vm_offset_t)thread_table, table_size * sizeof(thread_array_t));
   
   }
   if (task != mach_task_self()) {
      mach_port_deallocate(mach_task_self(), task);
   }
   return 0;
}



int main(int argc, const char * argv[])
{
   
   clock_t begin = clock();
   
   fileOperations();
   
   clock_t end = clock();
   results();
   double time_spent = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
   
   FILE * resultFile = fopen("results.txt", "a");
   fprintf(resultFile, " %ld\n", (long)time_spent);
   printf("wall time %ld\n", (long)time_spent);
   fclose(resultFile);
   
}

int results()
{
   
   struct rusage usage;
   int i = getrusage(RUSAGE_SELF, &usage);
   
   struct task_basic_info t_info;
   
   
   
   mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
   
   if (KERN_SUCCESS != task_info(mach_task_self(),
                                 TASK_BASIC_INFO, (task_info_t)&t_info,
                                 &t_info_count))
   {
      return -1;
   }
   
   CPUTime juan;
   
   get_cpu_time(&juan, 1);
   
   FILE * results = fopen("results.txt", "a");
   
   fprintf(results, "%ld %ld %ld %ld", t_info.resident_size / 1024, t_info.virtual_size/ 1024, (long)(juan.stime * 1000), (long)(juan.utime * 1000) );
   
   
   fclose(results);
   

   struct timeval userTime = usage.ru_utime;
   struct timeval sysTime = usage.ru_stime;
   
   
   return 0;
}


void fileOperations()
{
   
   int total = 10000;
   char readString[50];
   int i = 0;
   
   i=0;
   for(;i<total;i++)
   {
      snprintf(name, sizeof(name), "%d.txt", i);
      fp = fopen( name, "w");
      fclose(fp);
   }
   
   i=0;
   for(;i<total;i++)
   {
      snprintf(name, sizeof(name), "%d.txt", i);
      fp = fopen( name, "a");
      fprintf(fp, "hooking testing %d", i );
      fclose(fp);
   }
   
   i=0;
   for(;i<total;i++)
   {
      snprintf(name, sizeof(name), "%d.txt", i);
      fp = fopen( name, "r");
      fscanf(fp, "%s", readString);
      fclose(fp);
   }
   
   i=0;
   for(;i<total;i++)
   {
      snprintf(name, sizeof(name), "%d.txt", i);
      remove(name);
   }
   
}



