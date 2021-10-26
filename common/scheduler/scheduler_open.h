/**
 * scheduler_open
 * This header implements open scheduler functionality of SniperPlus. The class is extended from default "Pinned" scheduler.
 */


#ifndef __SCHEDULER_OPEN_H
#define __SCHEDULER_OPEN_H

#include "scheduler_pinned_base.h"
#include "thermalModel.h"
#include "performance_counters.h"
#include "policies/dvfspolicy.h"
#include "policies/mappingpolicy.h"
#include "policies/migrationpolicy.h"


class SchedulerOpen : public SchedulerPinnedBase {

	public:
		SchedulerOpen (ThreadManager *thread_manager); //This function is the constructor for Open System Scheduler.
		virtual void periodic(SubsecondTime time);
		virtual void threadSetInitialAffinity(thread_id_t thread_id);
		virtual bool threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask);
		virtual core_id_t threadCreate(thread_id_t thread_id);
		virtual void threadExit(thread_id_t thread_id, SubsecondTime time);

		

	private:
		int coreRows;
		int coreColumns;

		PerformanceCounters *performanceCounters;
		MappingPolicy *mappingPolicy = NULL;
		long mappingEpoch;
		void initMappingPolicy(String policyName);
		bool executeMappingPolicy(int taskID, SubsecondTime time);
		int getCoreNb(int y, int x);
		bool isAssignedToTask(int coreId);
		bool isAssignedToThread(int coreId);

		DVFSPolicy *dvfsPolicy = NULL;
		long dvfsEpoch;
		void initDVFSPolicy(String policyName);
		void executeDVFSPolicy();
		const int maxDVFSPatience = 0;
		std::vector<int> downscalingPatience; // can be used by the DVFS control loop to delay DVFS downscaling for very little violations
		std::vector<int> upscalingPatience; // can be used by the DVFS control loop to delay DVFS upscaling for very little violations
		bool delayDVFSTransition(int coreCounter, int oldFrequency, int newFrequency);
		void DVFSTransitionDelayed(int coreCounter, int oldFrequency, int newFrequency);
		void DVFSTransitionNotDelayed(int coreCounter);
		void setFrequency(int coreCounter, int frequency);
		ThermalModel *thermalModel;
		int minFrequency;
		int maxFrequency;
		int frequencyStepSize;

		MigrationPolicy *migrationPolicy = NULL;
		long migrationEpoch;
		void initMigrationPolicy(String policyName);
		void executeMigrationPolicy(SubsecondTime time);
		void migrateThread(thread_id_t thread_id, core_id_t core_id);

		std::string formatTime(SubsecondTime time);

		core_id_t getNextCore(core_id_t core_first);
		core_id_t getFreeCore(core_id_t core_first);

		const int m_interleaving;
		std::vector<bool> m_core_mask;
		core_id_t m_next_core;

		int setAffinity (thread_id_t thread_id);
		bool schedule (int taskID, bool isInitialCall, SubsecondTime time);
		
};

#endif // __SCHEDULER_OPEN_H