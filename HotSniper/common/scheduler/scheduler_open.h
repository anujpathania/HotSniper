/**
 * scheduler_open
 * This header implements open scheduler functionality of SniperPlus. The class is extended from default "Pinned" scheduler.
 */


#ifndef __SCHEDULER_OPEN_H
#define __SCHEDULER_OPEN_H

#include "scheduler_pinned_base.h"



class SchedulerOpen : public SchedulerPinnedBase {

	public:
		SchedulerOpen (ThreadManager *thread_manager); //This function is the constructor for Open System Scheduler.
		virtual void periodic(SubsecondTime time);
		virtual void threadSetInitialAffinity(thread_id_t thread_id);
		virtual bool threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask);
		virtual core_id_t threadCreate(thread_id_t thread_id);
		virtual void threadExit(thread_id_t thread_id, SubsecondTime time);
	private:
		double getPowerOfComponent (std::string component);
		double getPowerOfCore(int coreId);
		double getTemperatureOfComponent (std::string component);
		bool defaultLogic (int taskID, SubsecondTime time);
		int getCoreNb(int y, int x);
		int getCoreNb(std::pair<int,int> core);
		bool isAssignedToTask(int coreId);
		bool isAssignedToThread(int coreId);

		int coreRows;
		int coreColumns;

		core_id_t getNextCore(core_id_t core_first);
		core_id_t getFreeCore(core_id_t core_first);

		const int m_interleaving;
		std::vector<bool> m_core_mask;
		core_id_t m_next_core;

		int setAffinity (thread_id_t thread_id);
		bool schedule (int taskID, bool isInitialCall, SubsecondTime time);

		std::string schedulingLogic;
};

#endif // __SCHEDULER_ROAMING_H
