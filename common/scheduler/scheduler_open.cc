/**
 * scheduler_open
 * This class implements open scheduler functionality of SniperPlus.
 */


#include "scheduler_open.h"
#include "config.hpp"
#include "thread.h"
#include "core_manager.h"
#include "performance_model.h"
#include "magic_server.h"
#include "thread_manager.h"

#include "policies/dvfsMaxFreq.h"
#include "policies/dvfsFixedPower.h"
#include "policies/dvfsTSP.h"
#include "policies/dvfsTestStaticPower.h"
#include "policies/mapFirstUnused.h"

#include <iomanip>
#include <random>
#include <vector>
#include <queue>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

int k=0;

String queuePolicy; //Stores Queuing Policy for Open System from base.cfg.
String distribution; //Stores the arrival distribution of the open workload from base.cfg.

bool randomPriority; //Stores 1 if priority to be assigned randomly or 0 if priority is to be set by user explicitly from base.cfg.

int arrivalRate; //Stores the arrival rate of the workload from base.cfg.
int arrivalInterval; //Stores the arrival interval of the workload from base.cfg.

int numberOfTasks; //Stores the number of tasks in the open workload.
int numberOfCores; //Stores the number of cores in the system.

int coreRequirementTranslation (String compositionString);

//This data structure maintains the state of the tasks.
struct openTask {

	openTask(int taskIDInput, String taskNameInput) : taskID(taskIDInput), taskName(taskNameInput) {}

	int taskID;
        String taskName;
	bool waitingToSchedule = true;
	bool waitingInQueue = false;
	bool active = false;
	bool completed = false;
	int taskCoreRequirement = coreRequirementTranslation(taskName);
	UInt64 taskArrivalTime;
	UInt64 taskStartTime;
	UInt64 taskDepartureTime; 
	int priority;
	
};

vector <openTask> openTasks;

struct ComparePriority {															//custom comparator for waitingTaskQ
			bool operator()(openTask p1, openTask p2){
			// return "true" if "p1" is ordered
			// before "p2", for example:
				if(p1.priority == p2.priority){
					return p1.taskArrivalTime > p2.taskArrivalTime;
				}
				else{
					return p1.priority < p2.priority;
				}
			}
		};

struct ComparePriorityAQ {															//custom comparator for ActiveTaskQ
			bool operator()(openTask p1, openTask p2){
			// return "true" if "p1" is ordered
			// before "p2", for example:
				return p1.priority > p2.priority;
			}
		};

void showAQ(priority_queue<openTask, vector<openTask>, ComparePriorityAQ> gq)		//display taskID of tasks in ActiveTaskQ
{
    priority_queue<openTask, vector<openTask>, ComparePriorityAQ> g = gq;
    while (!g.empty()) {
        cout << g.top().taskID;
        g.pop();
    }
    cout << '\n';
};

void showtaskID(priority_queue<openTask, vector<openTask>, ComparePriority> gq)		//display taskID of tasks in waitingTaskQ
{
    priority_queue<openTask, vector<openTask>, ComparePriority> g = gq;
    while (!g.empty()) {
        cout << g.top().taskID;
        g.pop();
    }
    cout << '\n';
};


priority_queue<openTask, vector<openTask>, ComparePriority> waitingTaskQ;			//This priority queue holds the tasks in waiting queue in order of priority (with highest priority at top)
priority_queue<openTask, vector<openTask>, ComparePriorityAQ> ActiveTaskQ;		//This priority queue holds the tasks that are active in order of priority (with lowest priority at top)

		
//This data structure maintains the state of the cores.
struct systemCore {

	systemCore(int coreIDInput) : coreID(coreIDInput) {}

	int coreID;
	int assignedTaskID = -1; // -1 means core assigned to no task
	int assignedThreadID = -1;// -1 means core assigned to no thread.
};

vector <systemCore> systemCores;



/** SchedulerOpen
    Constructor for Open Scheduler
*/
SchedulerOpen::SchedulerOpen(ThreadManager *thread_manager)
   : SchedulerPinnedBase(thread_manager, SubsecondTime::NS(Sim()->getCfg()->getInt("scheduler/pinned/quantum")))
   , m_interleaving(Sim()->getCfg()->getInt("scheduler/pinned/interleaving"))
   , m_next_core(0) {

	// Initialize config constants
	minFrequency = (int)(1000 * Sim()->getCfg()->getFloat("scheduler/open/dvfs/min_frequency") + 0.5);
	maxFrequency = (int)(1000 * Sim()->getCfg()->getFloat("scheduler/open/dvfs/max_frequency") + 0.5);
	frequencyStepSize = (int)(1000 * Sim()->getCfg()->getFloat("scheduler/open/dvfs/frequency_step_size") + 0.5);
	dvfsEpoch = atol(Sim()->getCfg()->getString("scheduler/open/dvfs/dvfs_epoch").c_str());
	migrationEpoch = atol(Sim()->getCfg()->getString("scheduler/open/migration/epoch").c_str());

	m_core_mask.resize(Sim()->getConfig()->getApplicationCores());
	for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++) {
	       m_core_mask[core_id] = Sim()->getCfg()->getBoolArray("scheduler/open/core_mask", core_id);
  	}

	for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++) {
		downscalingPatience.push_back(maxDVFSPatience);
		upscalingPatience.push_back(maxDVFSPatience);
	}


	performanceCounters = new PerformanceCounters(Sim()->getCfg()->getString("general/output_dir").c_str(),
		"InstantaneousPower.log", "InstantaneousTemperature.log", "InstantaneousCPIStack.log");

	mappingEpoch = atol (Sim()->getCfg()->getString("scheduler/open/epoch").c_str());
	queuePolicy = Sim()->getCfg()->getString("scheduler/open/queuePolicy").c_str();
	distribution = Sim()->getCfg()->getString("scheduler/open/distribution").c_str();
	arrivalRate = atoi (Sim()->getCfg()->getString("scheduler/open/arrivalRate").c_str());
	arrivalInterval = atoi (Sim()->getCfg()->getString("scheduler/open/arrivalInterval").c_str());
	numberOfTasks = Sim()->getCfg()->getInt("traceinput/num_apps");
	numberOfCores = Sim()->getConfig()->getApplicationCores();
	randomPriority = Sim()->getCfg() ->getBool("scheduler/open/randompriority");
	

	

	coreRows = (int)sqrt(numberOfCores);
	while ((numberOfCores % coreRows) != 0) {
		coreRows -= 1;
	}
	coreColumns = numberOfCores / coreRows;
	if (coreRows * coreColumns != numberOfCores) {
		cout<<"\n[Scheduler] [Error]: Invalid system size: " << numberOfCores << ", expected rectangular-shaped system." << endl;
		exit (1);
	}
	double ambientTemperature = Sim()->getCfg()->getFloat("periodic_thermal/ambient_temperature");
    double maxTemperature = Sim()->getCfg()->getFloat("periodic_thermal/max_temperature");
    double inactivePower = Sim()->getCfg()->getFloat("periodic_thermal/inactive_power");
    double tdp = Sim()->getCfg()->getFloat("periodic_thermal/tdp");
	thermalModel = new ThermalModel((unsigned int)coreRows, (unsigned int)coreColumns, Sim()->getCfg()->getString("periodic_thermal/thermal_model"), ambientTemperature, maxTemperature, inactivePower, tdp);

	//Initialize the cores in the system.
	for (int coreIterator=0; coreIterator < numberOfCores; coreIterator++) {
		systemCores.push_back (coreIterator);
	}

	//Initialize the task state array.
	String benchmarks = Sim()->getCfg()->getString("traceinput/benchmarks");
	String benchmarksDelimiter = "+";
	for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
		openTasks.push_back (openTask (taskIterator,benchmarks.substr(0, benchmarks.find(benchmarksDelimiter))));
		benchmarks.erase(0, benchmarks.find(benchmarksDelimiter) + benchmarksDelimiter.length());		
	}						

	//Initialize the priority values (only when queuePolicy is "priority")
	if(queuePolicy == "priority"){

		if(randomPriority == true){
			for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
				openTasks[taskIterator].priority = rand()%10;
				
				cout << "[Scheduler]: Setting Priority for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << openTasks[taskIterator].priority << endl;
			}
		}
		else{
			for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			UInt64 priorityvalue = Sim()->getCfg()->getIntArray("scheduler/open/explicitPriorityValues", taskIterator);
			cout << "[Scheduler]: Setting Priority for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << priorityvalue << endl;
			openTasks[taskIterator].priority = priorityvalue;
			
			}
		}

	}

	//Initialize the task arrival time based on queuing policy.
	if (distribution == "uniform") {
		UInt64 time = 0;
		for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			if (taskIterator % arrivalRate == 0 && taskIterator != 0) time += arrivalInterval;  
			cout << "[Scheduler]: Setting Arrival Time for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << time << +" ns" << endl;
			openTasks[taskIterator].taskArrivalTime = time;
							
		}
	} else if (distribution == "explicit") {
		for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			UInt64 time = Sim()->getCfg()->getIntArray("scheduler/open/explicitArrivalTimes", taskIterator);
			cout << "[Scheduler]: Setting Arrival Time for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << time << +" ns" << endl;
			openTasks[taskIterator].taskArrivalTime = time;
			
		}
	} else if (distribution == "poisson") {
		// calculate Poisson-distributed arrival rates for the task.
		// The expected time between arrivales is the configured value "arrivalInterval".
		// The generation can either use a user-defined seed or generate a new seed for every execution.
		int seed = Sim()->getCfg()->getInt("scheduler/open/distributionSeed");
		if (seed == 0) {
			// Set a "truely random" seed
			std::random_device rd;
			seed = rd();
		}
		std::mt19937 generator(seed);
		generator(); // read one dummy value (first value was very like the seed: small seed -> small first arrival time, big seed -> big first arrival time. We do not want to have this.)
		double lambda = 1.0 / arrivalInterval;
		std::exponential_distribution<float> expdistribution(lambda);

		UInt64 time = 0;
		for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			if (taskIterator % arrivalRate == 0 && taskIterator != 0) {
				time += (UInt64)expdistribution(generator);
			}
			cout << "[Scheduler]: Setting Arrival Time for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << time << +" ns" << endl;
			openTasks[taskIterator].taskArrivalTime = time;
				
		}

		
	} else {
		cout << "\n[Scheduler] [Error]: Unknown Workload Arrival Distribution: '" << distribution << "'" << endl;
 		exit (1);
	}

	for(int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++){
		waitingTaskQ.push(openTasks[taskIterator]);
		cout << "Pushing Task " << taskIterator << " to the waitingTaskQ" << endl;
	}
	
	initMappingPolicy(Sim()->getCfg()->getString("scheduler/open/logic").c_str());
	initDVFSPolicy(Sim()->getCfg()->getString("scheduler/open/dvfs/logic").c_str());
	initMigrationPolicy(Sim()->getCfg()->getString("scheduler/open/migration/logic").c_str());
}

/** initMappingPolicy
 * Initialize the mapping policy to the policy with the given name
 */
void SchedulerOpen::initMappingPolicy(String policyName) {
	cout << "[Scheduler] [Info]: Initializing mapping policy" << endl;
	if (policyName == "first_unused") {
		vector<int> preferredCoresOrder;
		for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++) {
			int p = Sim()->getCfg()->getIntArray("scheduler/open/preferred_core", core_id);
			if (p != -1) {
				preferredCoresOrder.push_back(p);
			} else {
				break;
			}
		}
		mappingPolicy = new MapFirstUnused(coreRows, coreColumns, preferredCoresOrder);
	} //else if (policyName ="XYZ") {... } //Place to instantiate a new mapping logic. Implementation is put in "policies" package.
	else {
		cout << "\n[Scheduler] [Error]: Unknown Mapping Algorithm" << endl;
 		exit (1);
	}
}

/** initDVFSPolicy
 * Initialize the DVFS policy to the policy with the given name
 */
void SchedulerOpen::initDVFSPolicy(String policyName) {
	cout << "[Scheduler] [Info]: Initializing DVFS policy" << endl;
	if (policyName == "off") {
		dvfsPolicy = NULL;
	} else if (policyName == "maxFreq") {
		dvfsPolicy = new DVFSMaxFreq(performanceCounters, coreRows, coreColumns, maxFrequency);
	} else if (policyName == "testStaticPower") {
		dvfsPolicy = new DVFSTestStaticPower(performanceCounters, coreRows, coreColumns, minFrequency, maxFrequency);
	} else if (policyName == "fixedPower") {
		float perCorePowerBudget = Sim()->getCfg()->getFloat("scheduler/open/dvfs/fixed_power/per_core_power_budget");
		dvfsPolicy = new DVFSFixedPower(performanceCounters, coreRows, coreColumns, minFrequency, maxFrequency, frequencyStepSize, perCorePowerBudget);
	} else if (policyName == "tsp") {
		dvfsPolicy = new DVFSTSP(thermalModel, performanceCounters, coreRows, coreColumns, minFrequency, maxFrequency, frequencyStepSize);
	} //else if (policyName ="XYZ") {... } //Place to instantiate a new DVFS logic. Implementation is put in "policies" package.
	else {
		cout << "\n[Scheduler] [Error]: Unknown DVFS Algorithm" << endl;
 		exit (1);
	}
}

/** initMigrationPolicy
 * Initialize the migration policy to the policy with the given name
 */
void SchedulerOpen::initMigrationPolicy(String policyName) {
	cout << "[Scheduler] [Info]: Initializing migration policy" << endl;
	if (policyName == "off") {
		migrationPolicy = NULL;
	} //else if (policyName ="XYZ") {... } //Place to instantiate a new migration logic. Implementation is put in "policies" package.
	else {
		cout << "\n[Scheduler] [Error]: Unknown Migration Algorithm" << endl;
 		exit (1);
	}
}

/** taskFrontOfQueue
    Returns the ID of the task in front of queue. Place to implement a new queuing policy.
*/
int taskFrontOfQueue () {
	int IDofTaskInFrontOfQueue = -1;

	if (queuePolicy == "FIFO") {
		for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			if (openTasks [taskIterator].waitingInQueue) {
				IDofTaskInFrontOfQueue = taskIterator;
				break;
			}
		}
	}
	//else if (queuePolicy ="XYZ") {... } //Place to implement a new queuing policy.
	else if (queuePolicy == "priority"){ 
		SubsecondTime time = Sim()->getClockSkewMinimizationServer()->getGlobalTime();
		if ((openTasks [waitingTaskQ.top().taskID].waitingInQueue) && (openTasks[waitingTaskQ.top().taskID].taskArrivalTime <= time.getNS())){

			IDofTaskInFrontOfQueue = waitingTaskQ.top().taskID;

		}

		else {
			while ((!openTasks [waitingTaskQ.top().taskID].waitingInQueue) || (!(openTasks[waitingTaskQ.top().taskID].taskArrivalTime <= time.getNS()))){

				waitingTaskQ.pop();

			}
			IDofTaskInFrontOfQueue = waitingTaskQ.top().taskID;
		}
		
	}
	else {
	
		cout<<"\n[Scheduler] [Error]: Unknown Queuing Policy"<< endl;
 		exit (1);
	}

	return IDofTaskInFrontOfQueue;
}

/** numberOfFreeCores
    Returns number of free cores in the system.
*/
int numberOfFreeCores () {
	int freeCoresCounter = 0;
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
		if (systemCores[coreCounter].assignedTaskID == -1) {
			freeCoresCounter++;
		}
	}
	return freeCoresCounter;
}

/** numberOfTasksInQueue
    Returns the number of tasks in the queue.
*/
int numberOfTasksInQueue () {
	int tasksInQueue = 0;
	
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) 
		if (openTasks[taskCounter].waitingInQueue == true) 
			tasksInQueue++;
	
	return tasksInQueue;
}

/** numberOfTasksWaitingToSchedule
    Returns the number of tasks not yet entered into the queue.
*/
int numberOfTasksWaitingToSchedule () {
	int tasksWaitingToSchedule = 0;
	
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) 
		if (openTasks[taskCounter].waitingToSchedule == true) 
			tasksWaitingToSchedule++;

	
	return tasksWaitingToSchedule;
}

/** numberOfTasksCompleted
    Returns the number of tasks completed.
*/
int numberOfTasksCompleted () {
	int tasksCompleted = 0;
	
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) 
		if (openTasks[taskCounter].completed == true) 
			tasksCompleted++;

	
	return tasksCompleted;
}

/** numberOfActiveTasks
    Returns the number of active tasks.
*/
int numberOfActiveTasks () {
	int activeTasks = 0;
	
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) 
		if (openTasks[taskCounter].active == true) 
			activeTasks++;

	
	return activeTasks;
}

/** numberOfActiveTasks
    Returns the number of core required by all active tasks.
*/
int totalCoreRequirementsOfActiveTasks () {
	int coreRequirement = 0;
	
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) 
		if (openTasks[taskCounter].active == true) 
			coreRequirement += openTasks[taskCounter].taskCoreRequirement;

	return coreRequirement;
}

/** threadSetAffinity
    Original Sniper Function to set affinity of thread "thread_id" to set of CPUs.
*/
bool SchedulerOpen::threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask)
{
   if (m_thread_info.size() <= (size_t)thread_id)
      m_thread_info.resize(thread_id + 16);
   m_thread_info[thread_id].setExplicitAffinity();

   if (!mask)
   {
      // No mask given: free to schedule anywhere.
      for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
      {
         m_thread_info[thread_id].addAffinity(core_id);
      }
   }
   else
   {
      m_thread_info[thread_id].clearAffinity();

      for(unsigned int cpu = 0; cpu < 8 * cpusetsize; ++cpu)
      {
         if (CPU_ISSET_S(cpu, cpusetsize, mask))
         {
            LOG_ASSERT_ERROR(cpu < Sim()->getConfig()->getApplicationCores(), "Invalid core %d found in sched_setaffinity() mask", cpu);

            m_thread_info[thread_id].addAffinity(cpu);
         }
      }
   }

   // We're setting the affinity of a thread that isn't yet created. Do nothing else for now.
   if (thread_id >= (thread_id_t)Sim()->getThreadManager()->getNumThreads())
      return true;

   if (thread_id == calling_thread_id)
   {
      threadYield(thread_id);
   }
   else if (m_thread_info[thread_id].isRunning()                           // Thread is running
            && !m_thread_info[thread_id].hasAffinity(m_thread_info[thread_id].getCoreRunning())) // but not where we want it to
   {
      // Reschedule the thread as soon as possible
      m_quantum_left[m_thread_info[thread_id].getCoreRunning()] = SubsecondTime::Zero();
   }
   else if (m_threads_runnable[thread_id]                                  // Thread is runnable
            && !m_thread_info[thread_id].isRunning())                      // Thread is not running (we can't preempt it outside of the barrier)
   {
      core_id_t free_core_id = findFreeCoreForThread(thread_id);
      if (free_core_id != INVALID_THREAD_ID)                               // Thread's new core is free
      {
         // We have  been moved to a different core, and that core is free. Schedule us there now.
         Core *core = Sim()->getCoreManager()->getCoreFromID(free_core_id);
         SubsecondTime time = std::max(core->getPerformanceModel()->getElapsedTime(), Sim()->getClockSkewMinimizationServer()->getGlobalTime());
         reschedule(time, free_core_id, false);
      }
   }

   return true;
}

/** setAffinity
    This function finds free core for a thread with id "thread_id" and set its affinity to that core.

*/
int SchedulerOpen::setAffinity (thread_id_t thread_id) {
	int coreFound = -1;
	app_id_t app_id =  Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();

	for (int  i = 0; i<numberOfCores; i++) 
		if (systemCores[i].assignedTaskID == app_id && systemCores[i].assignedThreadID == -1) {
				coreFound = i;
				break;
		}

	
	if (coreFound == -1) {
		cout << "\n[Scheduler]: Setting Affinity for Thread " << thread_id << " from Task " << app_id << " to Invalid Core ID" << "\n" << endl;
		cpu_set_t my_set; 
		CPU_ZERO(&my_set); 
		CPU_SET(INVALID_CORE_ID, &my_set);		
		threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set); 
	} else {
		cout << "\n[Scheduler]: Setting Affinity for Thread " << thread_id << " from Task " << app_id << " to Core " << coreFound << "\n" << endl;
		cpu_set_t my_set; 
		CPU_ZERO(&my_set); 
		CPU_SET(coreFound, &my_set);
		threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set); 
		systemCores[coreFound].assignedThreadID = thread_id; 
	}

	return coreFound;
}


/** migrateThread
 * Move the given thread to the given core.
 */
void SchedulerOpen::migrateThread(thread_id_t thread_id, core_id_t core_id)
{
	int from_core_id = -1;
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
		if (systemCores[coreCounter].assignedThreadID == thread_id) {
			from_core_id = coreCounter;
			break;
		}
	}
	if (from_core_id == -1) {
		cout << "[Scheduler] [Error] could not find core of thread " << thread_id << endl;
		exit(1);
	}
	if (from_core_id == core_id) {
		cout << "[Scheduler] skipped moving thread " << thread_id << " to core " << core_id << " (already there)" << endl;
	} else {
		cout << "[Scheduler] moving thread " << thread_id << " from core " << from_core_id << " to core " << core_id << endl;
		if (isAssignedToTask(core_id)) {
			cout << "[Scheduler] [Error] core is already in use" << endl;
			exit(1);
		}
		
		cpu_set_t my_set; 
		CPU_ZERO(&my_set); 
		CPU_SET(core_id, &my_set);
		threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set); 

		systemCores[core_id].assignedTaskID = systemCores[from_core_id].assignedTaskID;
		systemCores[core_id].assignedThreadID = thread_id;

		systemCores[from_core_id].assignedTaskID = -1;
		systemCores[from_core_id].assignedThreadID = -1;
	}
}


/** getCoreNb
 * Return the number of the core at the given coordinates.
 */
int SchedulerOpen::getCoreNb(int y, int x) {
	if ((y < 0) || (y >= coreRows) || (x < 0) || (x >= coreColumns)) {
		cout << "[Scheduler][getCoreNb][Error]: Invalid core coordinates: " << y << ", " << x << endl;
		exit (1);
	}
	return y * coreColumns + x;
}

/** isAssignedToTask
 * Return whether the given core is assigned to a task.
 */
bool SchedulerOpen::isAssignedToTask(int coreId) {
	return systemCores[coreId].assignedTaskID != -1;
}

/** isAssignedToThread
 * Return whether the given core is assigned to a thread.
 */
bool SchedulerOpen::isAssignedToThread(int coreId) {
	return systemCores[coreId].assignedThreadID != -1;
}

bool SchedulerOpen::executeMappingPolicy(int taskID, SubsecondTime time) {
	vector<bool> availableCores(numberOfCores);
	vector<bool> activeCores(numberOfCores);
	for (int i = 0; i < numberOfCores; i++) {
		availableCores.at(i) = m_core_mask[i] && !isAssignedToTask(i);
		activeCores.at(i) = isAssignedToTask(i);
	}
	// get the cores
	vector<int> bestCores = mappingPolicy->map(openTasks[taskID].taskName, openTasks[taskID].taskCoreRequirement, availableCores, activeCores);
	if ((int)bestCores.size() < openTasks[taskID].taskCoreRequirement) {
		cout << "[Scheduler]: Policy returned too few cores, mapping failed." << endl;
		return false;
	}

	// assign the cores
	for (unsigned int i = 0; i < bestCores.size(); i++) {
		cout << "[Scheduler]: Assigning Core " << bestCores.at(i) << " to Task " << taskID << endl;
		systemCores[bestCores.at(i)].assignedTaskID = taskID;
	}

	return true;
}

/** schedule
    This function attempt to schedule a task with logic defined in base.cfg.
*/
bool SchedulerOpen::schedule (int taskID, bool isInitialCall, SubsecondTime time) {
	cout <<"\n[Scheduler]: Trying to schedule Task " << taskID << " at Time " << formatTime(time) << endl;

	bool mappingSuccesfull = false;

	if (openTasks [taskID].taskArrivalTime > time.getNS ()) {
		cout <<"\n[Scheduler]: Task " << taskID << " is not ready for execution. \n";	
		return false; //Task not ready for mapping.
	} 
	
	else {
		cout <<"\n[Scheduler]: Task " << taskID << " put into execution queue. \n";
		openTasks [taskID].waitingInQueue = true;
		openTasks [taskID].waitingToSchedule = false;
			
		if(waitingTaskQ.size()>1){
			openTask tempo = waitingTaskQ.top();
			waitingTaskQ.pop();
			
			if(tempo.taskID != waitingTaskQ.top().taskID){		//making sure that one task doesn't get pushed to waitingTaskQ twice
				waitingTaskQ.push(tempo);
			}
			else{
				waitingTaskQ.pop();
				waitingTaskQ.push(openTasks[tempo.taskID]);
			}
		}	
	}

	if (taskFrontOfQueue () != taskID) {

		cout <<"\n[Scheduler]: Task " << taskID << " is not in front of the queue. \n";	
		return false; //Not turn of this task to be mapped.
	}

	if (numberOfFreeCores () < openTasks[taskID].taskCoreRequirement) { //If priority queuing is adopted, then we need to check for it and readjust tasks if required
 		if((queuePolicy == "priority") && (openTasks [ActiveTaskQ.top().taskID].priority < openTasks[taskID].priority)){
		   	 while((numberOfFreeCores () < openTasks[taskID].taskCoreRequirement) && (openTasks[ActiveTaskQ.top().taskID].priority < openTasks[taskID].priority) ){
				for(int t=0; t < Sim()->getThreadManager()->getNumThreads() ; t++ ){
					app_id_t app_id =  Sim()->getThreadManager()->getThreadFromID(t)->getAppId();
					if(ActiveTaskQ.top().taskID == app_id ){
						 SubsecondTime time = Sim()->getClockSkewMinimizationServer()->getGlobalTime();
						
						for (int i = 0; i < numberOfCores; i++) {
							if (systemCores[i].assignedThreadID == t){
								systemCores[i].assignedThreadID = -1;
								cout << "\n[Scheduler]: Releasing Core " << i << " from Thread " << t << "\n";
								
								cpu_set_t my_set; 
								CPU_ZERO(&my_set); 
								CPU_SET(INVALID_CORE_ID, &my_set);
								Sim()->getThreadManager()->getLock();
								m_thread_info[t].clearAffinity();	
							}

						}
					
						if (t < numberOfTasks) {									
							

							for (int i = 0; i < numberOfCores; i++) {
								if (systemCores[i].assignedTaskID == app_id) {
									systemCores[i].assignedTaskID = -1;
								}
							}
						}				 	
					}
				}
				 	
				openTasks[ActiveTaskQ.top().taskID].active = false;
				openTasks[ActiveTaskQ.top().taskID].waitingInQueue = true;
				if(waitingTaskQ.top().taskID != ActiveTaskQ.top().taskID){
					waitingTaskQ.push(ActiveTaskQ.top());
				}
				ActiveTaskQ.pop();
				k++;
			}																								
	 	}
		 
		else{						
			cout <<"\n[Scheduler]: Not Enough Free Cores (" << numberOfFreeCores () << ") to Schedule the Task " << taskID << " with cores requirement " << openTasks[taskID].taskCoreRequirement  << endl;
			return false;
		}
	}

	mappingSuccesfull = executeMappingPolicy(taskID, time);
		if (mappingSuccesfull) {
			if (!isInitialCall) 
			cout << "\n[Scheduler]: Waking Task " << taskID << " at core " << setAffinity (taskID) << endl;
				
		openTasks [taskID].taskStartTime = time.getNS();
		
		openTasks [taskID].active = true;		
		ActiveTaskQ.push(openTasks[taskID]);
		openTasks [taskID].waitingInQueue = false;
		
		if((queuePolicy=="priority") && (waitingTaskQ.top().taskID == taskID)){
			waitingTaskQ.pop();
		}
			
		openTasks [taskID].waitingToSchedule = false;
	} 

	return mappingSuccesfull;

}

/** threadCreate
    This original Sniper function is called when a thread is created.
*/
core_id_t SchedulerOpen::threadCreate(thread_id_t thread_id) {
	app_id_t app_id =  Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();

	SubsecondTime time = Sim()->getClockSkewMinimizationServer()->getGlobalTime();

	cout << "\n[Scheduler]: Trying to map Thread  " << thread_id << " from Task " << app_id << " at Time " << formatTime(time) << endl;

	//thead_id 0 to numberOfTasks are first threads of tasks, which are all created together when the system starts.
	if (thread_id == 0) 
	{	
		
		if (!schedule (0, true, time)) 
		{	
			cout << "\n[Scheduler] [Error]: Task 0 must be mapped for simulation to work.\n";
			exit (1);
		} 

		
	} else if (thread_id > 0 && thread_id <  numberOfTasks) 
	{

		schedule (thread_id, true, time);

	}



	if (m_thread_info.size() <= (size_t)thread_id)
		m_thread_info.resize(m_thread_info.size() + 16);

	if (m_thread_info[thread_id].hasAffinity()) {
     		// Thread already has an affinity set at/before creation
   	}
	else {
      		threadSetInitialAffinity(thread_id);
   	}

   	// The first thread scheduled on this core can start immediately, the others have to wait
	setAffinity (thread_id);
   	core_id_t free_core_id = findFreeCoreForThread(thread_id);
   	if (free_core_id != INVALID_CORE_ID) {
      		m_thread_info[thread_id].setCoreRunning(free_core_id);
      		m_core_thread_running[free_core_id] = thread_id;
      		m_quantum_left[free_core_id] = m_quantum;
      		return free_core_id;
   	}
   	else {
	
		if (thread_id >= numberOfTasks && free_core_id == INVALID_CORE_ID) {

			cout <<"\n[Scheudler] [Error]: A non-intial Thread " << thread_id << " From Task " << app_id << " failed to get a core.\n";
			exit (1);
		}
	cout <<"\n[Scheudler]: Putting Thread " << thread_id << " From Task " << app_id << " to sleep.\n";
      	m_thread_info[thread_id].setCoreRunning(INVALID_CORE_ID);
      	return INVALID_CORE_ID;
   	}
}

/** fetchTasksIntoQueue
    This function pulls tasks into the openSystem Queue.
*/
void fetchTasksIntoQueue (SubsecondTime time) {

	if(queuePolicy == "priority"){
		for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) {
			if (openTasks [taskCounter].waitingToSchedule && openTasks [taskCounter].taskArrivalTime <= time.getNS ()) {
				
				cout <<"\n[Scheduler]: Task " << taskCounter << " put into execution queue. \n";
				openTasks [taskCounter].waitingInQueue = true;
				
				
					if(waitingTaskQ.top().taskID != taskCounter){
					waitingTaskQ.push(openTasks [taskCounter]);
					}
					
					if(waitingTaskQ.top().taskID == ActiveTaskQ.top().taskID){
						waitingTaskQ.pop();
					}
			
			
				openTasks [taskCounter].waitingToSchedule = false;

				cout << "Task " << taskCounter << " has been pushed to waitingTaskQ" << endl;
			}
		}
	}

	else{
		for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) {
			if (openTasks [taskCounter].waitingToSchedule && openTasks [taskCounter].taskArrivalTime <= time.getNS ()) {
				cout <<"\n[Scheduler]: Task " << taskCounter << " put into execution queue. \n";
				openTasks [taskCounter].waitingInQueue = true;
				openTasks [taskCounter].waitingToSchedule = false;
			}
		}
	}
}


/** threadExit
    This original Sniper function is called when a thread with "thread_id" exits.
*/
void SchedulerOpen::threadExit(thread_id_t thread_id, SubsecondTime time) {
	// If the running thread becomes unrunnable, schedule someone else
	if (m_thread_info[thread_id].isRunning())
		reschedule(time, m_thread_info[thread_id].getCoreRunning(), false);

	app_id_t app_id =  Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();
	cout << "\n[Scheduler]: Thread " << thread_id << " from Task "  << app_id << " Exiting at Time " << formatTime(time) << endl;

	for (int i = 0; i < numberOfCores; i++) {
		if (systemCores[i].assignedThreadID == thread_id) {
			systemCores[i].assignedThreadID = -1;
			cout << "\n[Scheduler]: Releasing Core " << i << " from Thread " << thread_id << "\n";
			
			cpu_set_t my_set; 
			CPU_ZERO(&my_set); 
			CPU_SET(INVALID_CORE_ID, &my_set);
			threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set);	
		}
		
	}


	if (thread_id < numberOfTasks) {
		
		cout << "\n[Scheduler]: Task " << app_id << " Finished." << "\n";

			for (int i = 0; i < numberOfCores; i++) {
				if (systemCores[i].assignedTaskID == app_id) {
					systemCores[i].assignedTaskID = -1;
					cout << "\n[Scheduler]: Releasing Core " << i << " from Task " << app_id << "\n";
				}
			}

			openTasks[app_id].taskDepartureTime = time.getNS();
			openTasks[app_id].completed = true;
			openTasks[app_id].active = false;
			if(queuePolicy == "priority"){
				priority_queue<openTask, vector<openTask>, ComparePriority> tempQ;

				if(ActiveTaskQ.top().taskID == app_id){

					ActiveTaskQ.pop();

				}
				else{
					while(ActiveTaskQ.top().taskID != app_id){

						tempQ.push(ActiveTaskQ.top());
						ActiveTaskQ.pop();

					}

					while(tempQ.size() != 0){

						ActiveTaskQ.push(tempQ.top());
						tempQ.pop();

					}
				}
			}
			
		cout << "\n[Scheduler][Result]: Task " << app_id << " (Response/Service/Wait) Time (ns) "  << " :\t" <<  time.getNS() - openTasks[app_id].taskArrivalTime << "\t" <<  time.getNS() - openTasks[app_id].taskStartTime << "\t" << openTasks[app_id].taskStartTime - openTasks[app_id].taskArrivalTime << "\n";
	
	}
	
	if (numberOfFreeCores () == numberOfCores && numberOfTasksWaitingToSchedule () != 0) {
		cout << "\n[Scheduler]: System Going Empty ... Prefetching Tasks\n"; //Without Prefectching Sniper will Deadlock or End Prematurely.

		if (numberOfTasksInQueue () != 0) {
			cout << "\n[Scheduler]: Prefetching Task from Queue\n";
			schedule (taskFrontOfQueue (), false, time);
		}
		else if (numberOfTasksWaitingToSchedule () != 0) {

			UInt64 timeJump = 0;

			UInt64 nextArrivalTime = 0;
			for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
				if (openTasks[taskIterator].waitingToSchedule) {
					if (nextArrivalTime == 0) { 
						nextArrivalTime = openTasks[taskIterator].taskArrivalTime;
					}
					else if (nextArrivalTime > openTasks[taskIterator].taskArrivalTime) {
						nextArrivalTime = openTasks[taskIterator].taskArrivalTime;
					}
				}

			}

			timeJump = nextArrivalTime - time.getNS();
			cout << "\n[Scheduler]: Readjusting Arrival Time by " << timeJump << " ns \n"; // This will not effect the result of response time as arrival time of all unscheduled tasks are adjusted relatively.

			for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
				if (openTasks[taskIterator].waitingToSchedule) {
					openTasks[taskIterator].taskArrivalTime -= timeJump;
					cout << "\n[Scheduler]: New Arrival Time from Task " << taskIterator << " set at " << openTasks[taskIterator].taskArrivalTime << " ns" <<  "\n"; 
				}
			}

			fetchTasksIntoQueue (time);

			schedule (taskFrontOfQueue (), false, time);

		}		
		

	}

	if (numberOfTasksCompleted ()  == numberOfTasks) {
		
		cout << "\n[Scheduler]: All tasks finished executing. \n";
		UInt64 averageResponseTime = 0;

		for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++){
			averageResponseTime += openTasks [taskCounter].taskDepartureTime - openTasks [taskCounter].taskArrivalTime;
		}


		cout << "\n[Scheduler][Result]: Average Response Time (ns) " << " :\t" <<  averageResponseTime/numberOfTasks << "\n\n";

	}


}



core_id_t SchedulerOpen::getNextCore(core_id_t core_id)
{
   while(true)
   {
      core_id += m_interleaving;
      if (core_id >= (core_id_t)Sim()->getConfig()->getApplicationCores())
      {
         core_id %= Sim()->getConfig()->getApplicationCores();
         core_id += 1;
         core_id %= m_interleaving;
      }
      if (m_core_mask[core_id])
         return core_id;
   }
}

core_id_t SchedulerOpen::getFreeCore(core_id_t core_first)
{
   core_id_t core_next = core_first;

   do
   {
      if (m_core_thread_running[core_next] == INVALID_THREAD_ID)
         return core_next;

      core_next = getNextCore(core_next);
   }
   while(core_next != core_first);

   return core_first;
}

void SchedulerOpen::threadSetInitialAffinity(thread_id_t thread_id)
{
   core_id_t core_id = getFreeCore(m_next_core);
   m_next_core = getNextCore(core_id);

   m_thread_info[thread_id].setAffinitySingle(core_id);
}





/** coreRequirementTranslation
    This function gets the worst-case core requirement of a task.
*/
int coreRequirementTranslation (String compositionString) {
	int del1 = compositionString.find('-');
	int del2 = compositionString.find('-', del1 + 1);
	int del3 = compositionString.find('-', del2 + 1);

	String suite = compositionString.substr(0, del1);
	String benchmark = compositionString.substr(del1 + 1, del2 - del1 - 1);
	String input = compositionString.substr(del2 + 1, del3 - del2 - 1);
	int parallelism = atoi(compositionString.substr(del3 + 1, compositionString.length() - del3 - 1).c_str());

	if (parallelism < 1) {
		cout << "\n[Scheduler] [Error]: Can't find core requirement of " << compositionString << "(parallelism < 1). Please add the profile." << endl;		
		exit (1);
	}
	vector<int> requirements;
	if (suite == "parsec") {
		if (benchmark == "blackscholes") {
			int t[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "bodytrack") {
			int t[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "canneal") {
			int t[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "dedup") {
			int t[] = {4, 7, 10, 13, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "ferret") {
			int t[] = {7, 11, 15};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "fluidanimate") {
			int t[] = {2, 3, 0, 5, 0, 0, 0, 9};  // zeros are  placeholders
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "streamcluster") {
			int t[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "swaptions") {
			int t[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "x264") {
			int t[] = {1, 3, 4, 5, 6, 7, 8, 9};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		}
	} else if (suite == "splash2") {
		if (benchmark == "barnes") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "cholesky") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "fft") {
			int t[] = {1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16};  // zeros are  placeholders
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "fmm") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "lu.cont") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "lu.ncont") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "ocean.cont") {
			int t[] = {1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16};  // zeros are  placeholders
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "ocean.ncont") {
			int t[] = {1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16};  // zeros are  placeholders
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "radiosity") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "radix") {
			int t[] = {1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16};  // zeros are  placeholders
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "raytrace") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "water.nsq") {
			int t[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		} else if (benchmark == "water.sp") {
			int t[] = {1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16};  // zeros are  placeholders, other parallelism values run but are suboptimal -> don't allow in the first place
			requirements.insert(requirements.end(), std::begin(t), std::end(t));
		}
	} else {
		cout <<"\n[Scheduler] [Error]: Can't find core requirement of " << compositionString << " (only PARSEC and SPLASH2 are implemented). Please add the profile." << endl;		
		exit (1);
	}

	if (parallelism - 1 < (int)requirements.size()) {
		return requirements.at(parallelism - 1);
	} else {
		cout <<"\n[Scheduler] [Error]: Can't find core requirement of " << compositionString << ". Please add the profile." << endl;		
		exit (1);
	}
}


/**
 * Return whether the DVFS control loop should be patient and delay the DVFS scaling.
 */
bool SchedulerOpen::delayDVFSTransition(int coreCounter, int oldFrequency, int newFrequency) {
	return ((newFrequency == oldFrequency - frequencyStepSize) && (downscalingPatience.at(coreCounter) > 0))
		 || ((newFrequency == oldFrequency + frequencyStepSize) && (upscalingPatience.at(coreCounter) > 0));
}

/**
 * Notify that a DVFS transition was delayed
 */
void SchedulerOpen::DVFSTransitionDelayed(int coreCounter, int oldFrequency, int newFrequency) {
	if (newFrequency == oldFrequency - frequencyStepSize) {
		cout << "DVFS transition delayed (current patience: " << downscalingPatience.at(coreCounter) << ")" << endl;
		downscalingPatience.at(coreCounter) -= 1;
	} else if (newFrequency == oldFrequency + frequencyStepSize) {
		cout << "DVFS transition delayed (current patience: " << upscalingPatience.at(coreCounter) << ")" << endl;
		upscalingPatience.at(coreCounter) -= 1;
	}
}

/**
 * Notify that a DVFS transition was not delayed
 */
void SchedulerOpen::DVFSTransitionNotDelayed(int coreCounter) {
	downscalingPatience.at(coreCounter) = maxDVFSPatience;
	upscalingPatience.at(coreCounter) = maxDVFSPatience;
}

/**
 * Set the frequency for a core.
 */
void SchedulerOpen::setFrequency(int coreCounter, int frequency) {
	int oldFrequency = Sim()->getMagicServer()->getFrequency(coreCounter);

	if (frequency > oldFrequency + 1000) {
		frequency = oldFrequency + 1000;
	}
	if (frequency < minFrequency) {
		frequency = minFrequency;
	}
	if (frequency > maxFrequency) {
		frequency = maxFrequency;
	}

	if (delayDVFSTransition(coreCounter, oldFrequency, frequency)) {
		DVFSTransitionDelayed(coreCounter, oldFrequency, frequency);
	} else {
		DVFSTransitionNotDelayed(coreCounter);
		if (frequency != oldFrequency) {
			Sim()->getMagicServer()->setFrequency(coreCounter, frequency);
		}
	}
}


/** executeDVFSPolicy
 * Set DVFS levels according to the used policy.
 */
void SchedulerOpen::executeDVFSPolicy() {
	std::vector<int> oldFrequencies;
	std::vector<bool> activeCores;
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
		oldFrequencies.push_back(Sim()->getMagicServer()->getFrequency(coreCounter));
	    static bool reserved_cores_are_active = Sim()->getCfg()->getBool("scheduler/open/dvfs/reserved_cores_are_active");
		activeCores.push_back(reserved_cores_are_active ? isAssignedToTask(coreCounter) : isAssignedToThread(coreCounter));
	}
	vector<int> frequencies = dvfsPolicy->getFrequencies(oldFrequencies, activeCores);
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
		setFrequency(coreCounter, frequencies.at(coreCounter));
	}
	performanceCounters->notifyFreqsOfCores(frequencies);
}

/** executeMigrationPolicy
 * Perform migration according to the used policy.
 */
void SchedulerOpen::executeMigrationPolicy(SubsecondTime time) {
	std::vector<int> taskIds;
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
		taskIds.push_back(systemCores.at(coreCounter).assignedTaskID);
	}
	std::vector<bool> activeCores;
	for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
	    static bool reserved_cores_are_active = Sim()->getCfg()->getBool("scheduler/open/dvfs/reserved_cores_are_active");
		activeCores.push_back(reserved_cores_are_active ? isAssignedToTask(coreCounter) : isAssignedToThread(coreCounter));
	}
	std::vector<migration> migrations = migrationPolicy->migrate(time, taskIds, activeCores);

	for (migration &migration : migrations) {
		if (systemCores.at(migration.fromCore).assignedTaskID == -1) {
			cout << "\n[Scheduler][Error]: Migration Policy ordered migration from unused core.\n";		
			exit (1);
		}

		if (migration.swap) {
			if (systemCores.at(migration.toCore).assignedTaskID == -1) {
				cout << "\n[Scheduler][Error]: Migration Policy ordered swap with unused core.\n";		
				exit (1);
			}
			int taskFrom = systemCores.at(migration.fromCore).assignedTaskID;
			int threadFrom = systemCores.at(migration.fromCore).assignedThreadID;
			int taskTo = systemCores.at(migration.toCore).assignedTaskID;
			int threadTo = systemCores.at(migration.toCore).assignedThreadID;

			if (threadFrom != -1) {
				cout << "[Scheduler] moving thread " << threadFrom << " from core " << migration.fromCore << " to core " << migration.toCore << endl;
				cpu_set_t my_set;
				CPU_ZERO(&my_set);
				CPU_SET(migration.toCore, &my_set);
				threadSetAffinity(INVALID_THREAD_ID, threadFrom, sizeof(cpu_set_t), &my_set); 
			}
			if (threadTo != -1) {
				cout << "[Scheduler] moving thread " << threadTo << " from core " << migration.toCore << " to core " << migration.fromCore << endl;
				cpu_set_t my_set;
				CPU_ZERO(&my_set);
				CPU_SET(migration.fromCore, &my_set);
				threadSetAffinity(INVALID_THREAD_ID, threadTo, sizeof(cpu_set_t), &my_set);
			}
			systemCores.at(migration.toCore).assignedTaskID = taskFrom;
			systemCores.at(migration.toCore).assignedThreadID = threadFrom;
			systemCores.at(migration.fromCore).assignedTaskID = taskTo;
			systemCores.at(migration.fromCore).assignedThreadID = threadTo;
		} else {
			if (systemCores.at(migration.toCore).assignedTaskID != -1) {
				cout << "\n[Scheduler][Error]: Migration Policy ordered migration to already used core.\n";
				exit (1);
			}
			int thread = systemCores.at(migration.fromCore).assignedThreadID;
			if (thread != -1) {
				migrateThread(thread, migration.toCore);
			} else {
				systemCores.at(migration.toCore).assignedTaskID = systemCores.at(migration.fromCore).assignedTaskID;
				systemCores.at(migration.fromCore).assignedTaskID = -1;
			}
		}
	}
}


/** periodic
    This function is called periodically by Sniper at Interval of 100ns.
*/
void SchedulerOpen::periodic(SubsecondTime time) {
	if (time.getNS () % 1000000 == 0) { //Error Checking at every 1ms. Can be faster but will have overhead in simulation time.
		cout << "\n[Scheduler]: Time " << formatTime(time) << " [Active Tasks =  " << numberOfActiveTasks () << " | Completed Tasks = " <<  numberOfTasksCompleted () << " | Queued Tasks = "  << numberOfTasksInQueue () << " | Non-Queued Tasks  = " <<  numberOfTasksWaitingToSchedule () <<  " | Free Cores = " << numberOfFreeCores () << " | Active Tasks Requirements = " << totalCoreRequirementsOfActiveTasks () << " ] \n" << endl;

		// showtaskID(waitingTaskQ);
		// showAQ(ActiveTaskQ);
		//Following error checking code makes sure that the system state is not messed up.

		if (numberOfCores - totalCoreRequirementsOfActiveTasks () != numberOfFreeCores ()) {
			cout <<"\n[Scheduler] [Error]: Number of Free Cores + Number of Active Tasks Requirements != Number Of Cores.\n";		
			exit (1);
		}

		if (numberOfActiveTasks () + numberOfTasksCompleted () + numberOfTasksInQueue () + numberOfTasksWaitingToSchedule () != numberOfTasks) {
			cout <<"\n[Scheduler] [Error]: Task State Does Not Match.\n";		
			exit (1);
		}
	}

	if ((migrationPolicy != NULL) && (time.getNS() % migrationEpoch == 0)) {
		cout << "\n[Scheduler]: Migration invoked at " << formatTime(time) << endl;

		executeMigrationPolicy(time);
	}

	if ((dvfsPolicy != NULL) && (time.getNS() % dvfsEpoch == 0)) {
		cout << "\n[Scheduler]: DVFS Control Loop invoked at " << formatTime(time) << endl;

		executeDVFSPolicy();

		std::vector<int> frequencies;
		for (int coreCounter = 0; coreCounter < numberOfCores; coreCounter++) {
			frequencies.push_back(Sim()->getMagicServer()->getFrequency(coreCounter));
		}
	}

	if (time.getNS () % mappingEpoch == 0) {
		
		cout << "\n[Scheduler]: Scheduler Invoked at " << formatTime(time) << "\n" << endl;

		fetchTasksIntoQueue (time);
				


		while (	numberOfTasksInQueue () != 0) {	
			if (!schedule (taskFrontOfQueue (), false,time)) break; //Scheduler can't map the task in front of queue.
		}

		cout << "[Scheduler]: Current mapping:" << endl;

		for (int y = 0; y < coreRows; y++) {
			for (int x = 0; x < coreColumns; x++) {
				if (x > 0) {
					cout << " ";
				}
				int coreId = getCoreNb(y, x);
				if (!isAssignedToTask(coreId)) {
					cout << "  . ";
				} else {
					if (systemCores[coreId].assignedTaskID < 10) {
						cout << " ";
					}

					char marker1 = '?';
					char marker2 = '?';
					if (isAssignedToThread(coreId)) {
						Core::State state = m_thread_manager->getThreadState(systemCores[coreId].assignedThreadID);
						if (state == Core::State::RUNNING) {
							marker1 = '*';
							marker2 = '*';
						} else {
							marker1 = '-';
							marker2 = '-';
						}
					} else {
						marker1 = '(';
						marker2 = ')';
					}

					cout << marker1 << systemCores[coreId].assignedTaskID << marker2;
				}
			}
			cout << endl;
		}
	}


	SubsecondTime delta = time - m_last_periodic;

	for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id) {
		if (delta > m_quantum_left[core_id] || m_core_thread_running[core_id] == INVALID_THREAD_ID) {
		         reschedule(time, core_id, true);
		}
		else {
			m_quantum_left[core_id] -= delta;
		}
	}

	m_last_periodic = time;
}

std::string formatLong(long l) {
	std::stringstream ss;
	if (l < 1000) {
		ss << l;
	} else {
		long curr = l % 1000;
		ss << formatLong(l / 1000) << '.' << std::setfill('0') << std::setw(3) << curr;
	}
	return ss.str();
}

std::string SchedulerOpen::formatTime(SubsecondTime time) {
	std::stringstream ss;
	ss << formatLong(time.getNS()) << " ns";
	return ss.str();
}
