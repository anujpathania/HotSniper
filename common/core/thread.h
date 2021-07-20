#ifndef __THREAD_H
#define __THREAD_H

#include "cond.h"
#include "subsecond_time.h"
#include <iostream>
using namespace std;

class Core;
class SyscallMdl;
class SyncClient;
class RoutineTracerThread;

class Thread
{
   public:
      typedef UInt64 (*va2pa_func_t)(UInt64 arg, UInt64 va);

   private:
      thread_id_t m_thread_id;
      app_id_t m_app_id;
      tile_id_t m_tile_id;
      String m_name;
      bool m_secure;
      ConditionVariable m_cond;
      SubsecondTime m_wakeup_time;
      void *m_wakeup_msg;
      Core *m_core;
      SyscallMdl *m_syscall_model;
      SyncClient *m_sync_client;
      RoutineTracerThread *m_rtn_tracer;
      va2pa_func_t m_va2pa_func;
      UInt64 m_va2pa_arg;
      UInt32 m_shared_slots;
      UInt64 m_last_instr;
      double m_periodic_performance;
      void setTile();
      

   public:
      Thread(thread_id_t thread_id, app_id_t app_id, String app_name="X", bool secure=false);
      ~Thread();

      struct {
         pid_t tid;
         IntPtr tid_ptr;
         bool clear_tid;
      } m_os_info;

      thread_id_t getId() const { return m_thread_id; }
      app_id_t getAppId() const { return m_app_id; }
      tile_id_t getTileId() const {return m_tile_id; }

      String getName() const { return m_name; }
      void setName(String name) { m_name = name; }
      void setSecure() {m_secure = true; }
      bool isSecure() const { return m_secure; }
      SyncClient *getSyncClient() const { return m_sync_client; }
      RoutineTracerThread* getRoutineTracer() const { return m_rtn_tracer; }

      void setVa2paFunc(va2pa_func_t va2pa_func, UInt64 m_va2pa_arg);
      UInt64 va2pa(UInt64 logical_address) const
      {
         if (m_va2pa_func)
            return m_va2pa_func(m_va2pa_arg, logical_address);
         else
            return logical_address;
      }

      SubsecondTime wait(Lock &lock)
      {
         m_wakeup_msg = NULL;
         m_cond.wait(lock);
         return m_wakeup_time;
      }
      void signal(SubsecondTime time, void* msg = NULL)
      {
         m_wakeup_time = time;
         m_wakeup_msg = msg;
         m_cond.signal();
      }
      SubsecondTime getWakeupTime() const { return m_wakeup_time; }
      void *getWakeupMsg() const { return m_wakeup_msg; }

      Core* getCore() const { return m_core; }
      void setCore(Core* core);

      void incSharedSlots();
      int getSharedSlots() const { return m_shared_slots; }

      double getPeriodicPerformance() const { return m_periodic_performance; }
      UInt64 getLastInstructionCount() const { return m_last_instr; }
      void updatePeriodicPerformance(UInt64 instructions, UInt64 time_interval);


      bool reschedule(SubsecondTime &time, Core *current_core);
      bool updateCoreTLS(int threadIndex = -1);

      SyscallMdl *getSyscallMdl() { return m_syscall_model; }
};

#endif // __THREAD_H