/**
 * scheduler_open
 * This class implements open scheduler functionality of SniperPlus.
 */


#include "scheduler_open.h"
#include "config.hpp"
#include "thread.h"
#include "core_manager.h"
#include "performance_model.h"

#include <iomanip>
#include <vector>

using namespace std;

long schedulingEpoch; //Stores the scheduling epoch defined by the user.

String queuePolicy; //Stores Queuing Policy for Open System from base.cfg.
String distribution; //Stores the arrival distribution of the open workload from base.cfg.

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

};

vector <openTask> openTasks;

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

	m_core_mask.resize(Sim()->getConfig()->getApplicationCores());
	for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++) {
	       m_core_mask[core_id] = Sim()->getCfg()->getBoolArray("scheduler/open/core_mask", core_id);
  	}


	schedulingEpoch = atol (Sim()->getCfg()->getString("scheduler/open/epoch").c_str());
	queuePolicy = Sim()->getCfg()->getString("scheduler/open/queuePolicy").c_str();
	distribution = Sim()->getCfg()->getString("scheduler/open/distribution").c_str();
	schedulingLogic = Sim()->getCfg()->getString("scheduler/open/logic").c_str();
	arrivalRate = atoi (Sim()->getCfg()->getString("scheduler/open/arrivalRate").c_str());
	arrivalInterval = atoi (Sim()->getCfg()->getString("scheduler/open/arrivalInterval").c_str());
	numberOfTasks = Sim()->getCfg()->getInt("traceinput/num_apps");
	numberOfCores = Sim()->getConfig()->getApplicationCores();

	coreRows = 1;
	while (coreRows * coreRows < numberOfCores) {
		// assume rectangular layout
		coreRows += 1;
	}
	coreColumns = numberOfCores / coreRows;
	if (coreRows * coreColumns != numberOfCores) {
		cout<<"\n[Scheduler] [Error]: Invalid system size: " << numberOfCores << ", expected square-shaped system." << endl;
		exit (1);
	}

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

	//Initialize the task arrival time based on queuing policy.
	if (distribution == "uniform") {
		UInt64 time = 0;
		for (int taskIterator = 0; taskIterator < numberOfTasks; taskIterator++) {
			if (taskIterator % arrivalRate == 0 && taskIterator != 0) time += arrivalInterval;  
			cout << "\n[Scheduler]: Setting Arrival Time  for Task " << taskIterator << " (" + openTasks[taskIterator].taskName + ")" << " to " << time << +" ns" << endl;
			openTasks[taskIterator].taskArrivalTime = time;
		}
	}
	else {
		cout << "\n[Scheduler] [Error]: Unknown Workload Arrival Distribution" << endl;
 		exit (1);
	}
}


/** getPowerOfComponent
    Returns the latest power consumption of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double SchedulerOpen::getPowerOfComponent (string component) {

	ifstream powerLogFile("InstantaneousPower.log");


	
    	string header;
	string footer;

  	if (powerLogFile.good()) {
    		getline(powerLogFile, header);
    		getline(powerLogFile, footer);
  	}

	
	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {

		std::string value;
		getline(issFooter, value, '\t');

		if (token == component) {
		
			return stod (value);

		}

	}


	return -1;

}

/** getPowerOfCore
 * Return the latest total power consumption of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double SchedulerOpen::getPowerOfCore(int coreId) {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getPowerOfComponent(component);
}


/** getTemperatureOfComponent
    Returns the latest temperature of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double SchedulerOpen::getTemperatureOfComponent (string component) {
	ifstream powerLogFile("InstantaneousTemperature.log");
	string header;
	string footer;

  	if (powerLogFile.good()) {
    		getline(powerLogFile, header);
    		getline(powerLogFile, footer);
  	}

	
	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {
		std::string value;
		getline(issFooter, value, '\t');

		if (token == component) {
			return stod (value);
		}
	}

	return -1;
}

/**
 * Get a performance metric for the given core.
 * Available performance metrics can be checked in InstantaneousPerformanceCounters.log
 */
double SchedulerOpen::getPerformanceCounterOfCore(int coreId, std::string metric) {
	ifstream performanceCounterLogFile("InstantaneousPerformanceCounters.log");
    string line;

	// first find the line in the logfile that contains the desired metric
	bool metricFound = false;
	while (!metricFound) {
  		if (performanceCounterLogFile.good()) {
			getline(performanceCounterLogFile, line);
			metricFound = (line.substr(0, metric.size()) == metric);
		} else {
			return -1;
		}
	}
	
	// then split the (coreId + 1)-th value from this line (first value is metric name)
 	std::istringstream issLine(line);

	std::string value;
	for (int i = 0; i < coreId + 2; i++) {
		getline(issLine, value, '\t');
	}

	return stod(value);
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
         // We have just been moved to a different core, and that core is free. Schedule us there now.
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
		cout << "\n[Scheduler]: Setting Affinity for Thread " << thread_id << " from Task " << app_id << " to Invalid Core ID" <<"\n"<<endl;
		cpu_set_t my_set; 
		CPU_ZERO(&my_set); 
		CPU_SET(INVALID_CORE_ID, &my_set);		
		threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set); 
	} else {
		cout << "\n[Scheduler]: Setting Affinity for Thread " << thread_id << " from Task " << app_id << " to Core " << coreFound << "\n" <<endl;
		cpu_set_t my_set; 
		CPU_ZERO(&my_set); 
		CPU_SET(coreFound, &my_set);
		threadSetAffinity(INVALID_THREAD_ID, thread_id, sizeof(cpu_set_t), &my_set); 
		systemCores[coreFound].assignedThreadID = thread_id; 
	}

	return coreFound;
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
int SchedulerOpen::getCoreNb(pair<int,int> core) {
	return getCoreNb(core.first, core.second);
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

/** defaultLogic
    This function goes through all free cores assign the first free ones found to the task.
*/
bool SchedulerOpen::defaultLogic (int taskID, SubsecondTime time) {
	int coresAssigned = 0;
	vector <int> freeCoresIndices;

	for (int i = 0; i <numberOfCores; i++) {
		bool available = m_core_mask[i];
		if (available) {
			if (!isAssignedToTask(i)) {
				freeCoresIndices.push_back (i);
			}
		}
	}

	int freeCoreCounter = 0;
	while (coresAssigned != openTasks[taskID].taskCoreRequirement) {
		cout <<"\n[Scheduler][Default]: Assigning Core " << freeCoresIndices[freeCoreCounter] << " to Task " << taskID << "\n"<<endl;
		systemCores [freeCoresIndices[freeCoreCounter]].assignedTaskID = taskID;
		freeCoreCounter++;
		coresAssigned++;
	}

	if (coresAssigned != openTasks[taskID].taskCoreRequirement) {
		
		cout <<"\n[Scheduler][Non-Contig][Error]: Default Mapping should have been sucessfull.\n";		
		exit (1);
	}

	return true;
}


/** schedule
    This function attempt to schedule a task with logic defined in base.cfg.
*/
bool SchedulerOpen::schedule (int taskID, bool isInitialCall, SubsecondTime time) {


	cout <<"\n[Scheduler]: Trying to schedule Task " << taskID << " at Time " << time.getNS() << " ns" << endl;

	bool mappingSuccesfull = false;

	if (openTasks [taskID].taskArrivalTime > time.getNS ()) {
		cout <<"\n[Scheduler]: Task " << taskID << " is not ready for execution. \n";	
		return false; //Task not ready for mapping.
	} else {
		cout <<"\n[Scheduler]: Task " << taskID << " put into execution queue. \n";
		openTasks [taskID].waitingInQueue = true;
		openTasks [taskID].waitingToSchedule = false;
	}

	if (taskFrontOfQueue () != taskID) {

		cout <<"\n[Scheduler]: Task " << taskID << " is not in front of the queue. \n";	
		return false; //Not turn of this task to be mapped.
	}

	if (numberOfFreeCores () < openTasks[taskID].taskCoreRequirement) {

		cout <<"\n[Scheduler]: Not Enough Free Cores (" << numberOfFreeCores () << ") to Schedule the Task " << taskID << " with cores requirement " << openTasks[taskID].taskCoreRequirement  << endl;
		return false;

	}

	if (schedulingLogic == "default") {
		mappingSuccesfull = defaultLogic (taskID, time);
	} //else if (schedulingLogic ="XYZ") {... } //Place to implement a new scheduling logic. 
	else {
		cout << "\n[Scheduler] [Error]: Unknown Scheduling Algorithm"<< endl;
 		exit (1);
	}

	if (mappingSuccesfull) {
		if (!isInitialCall) 
			cout << "\n[Scheduler]: Waking Task " << taskID << " at core " << setAffinity (taskID) << endl;
		openTasks [taskID].taskStartTime = time.getNS();
		openTasks [taskID].active = true;
		openTasks [taskID].waitingInQueue = false;
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

	cout << "\n[Scheduler]: Trying to map Thread  " << thread_id << " from Task " << app_id << " at Time " << time.getNS() << " ns" << endl;

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
	for (int taskCounter = 0; taskCounter < numberOfTasks; taskCounter++) {
		if (openTasks [taskCounter].waitingToSchedule && openTasks [taskCounter].taskArrivalTime <= time.getNS ()) {
			cout <<"\n[Scheduler]: Task " << taskCounter << " put into execution queue. \n";
			openTasks [taskCounter].waitingInQueue = true;
			openTasks [taskCounter].waitingToSchedule = false;
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
	cout << "\n[Scheduler]: Thread " << thread_id << " from Task "  << app_id << " Exiting at Time " << time.getNS() << " ns" << endl;

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


	int requirement = 0;
	
	
	

	if ((compositionString == "parsec-blackscholes-simsmall-1") || (compositionString == "parsec-blackscholes-simmedium-1") || (compositionString == "parsec-blackscholes-test-1"))
		requirement = 2;
	else if ((compositionString == "parsec-blackscholes-simsmall-2") || (compositionString == "parsec-blackscholes-simmedium-2"))
		requirement = 3;
	else if ((compositionString == "parsec-blackscholes-simsmall-3") || (compositionString == "parsec-blackscholes-simmedium-3"))
		requirement = 4;
	else if (compositionString == "parsec-blackscholes-simsmall-4")
		requirement = 5;
	else if (compositionString == "parsec-blackscholes-simsmall-5")
		requirement = 6;
	else if (compositionString == "parsec-blackscholes-simsmall-6")
		requirement = 7;
	else if (compositionString == "parsec-blackscholes-simsmall-7")
		requirement = 8;
	else if (compositionString == "parsec-blackscholes-simsmall-8")
		requirement = 9;
	else if (compositionString == "parsec-blackscholes-simsmall-9")
		requirement = 10;
	else if (compositionString == "parsec-blackscholes-simsmall-10")
		requirement = 11;
	else if (compositionString == "parsec-blackscholes-simsmall-11")
		requirement = 12;
	else if (compositionString == "parsec-blackscholes-simsmall-12")
		requirement = 13;
	else if (compositionString == "parsec-blackscholes-simsmall-13")
		requirement = 14;
	else if (compositionString == "parsec-blackscholes-simsmall-14")
		requirement = 15;
	else if (compositionString == "parsec-blackscholes-simsmall-15")
		requirement = 16;

	else if (compositionString == "parsec-bodytrack-simsmall-1" || compositionString == "parsec-bodytrack-test-1")
		requirement = 3;
	else if (compositionString == "parsec-bodytrack-simsmall-2")
		requirement = 4;
	else if (compositionString == "parsec-bodytrack-simsmall-3")
		requirement = 5;
	else if (compositionString == "parsec-bodytrack-simsmall-4")
		requirement = 6;
	else if (compositionString == "parsec-bodytrack-simsmall-5")
		requirement = 7;
	else if (compositionString == "parsec-bodytrack-simsmall-6")
		requirement = 8;
	else if (compositionString == "parsec-bodytrack-simsmall-7")
		requirement = 9;
	else if (compositionString == "parsec-bodytrack-simsmall-8")
		requirement = 10;
	else if (compositionString == "parsec-bodytrack-simsmall-9")
		requirement = 11;
	else if (compositionString == "parsec-bodytrack-simsmall-10")
		requirement = 12;
	else if (compositionString == "parsec-bodytrack-simsmall-11")
		requirement = 13;
	else if (compositionString == "parsec-bodytrack-simsmall-12")
		requirement = 14;
	else if (compositionString == "parsec-bodytrack-simsmall-13")
		requirement = 15;
	else if (compositionString == "parsec-bodytrack-simsmall-14")
		requirement = 16;





	else if (compositionString == "parsec-canneal-simsmall-1")
		requirement = 2;
	else if (compositionString == "parsec-canneal-simsmall-2")
		requirement = 3;
	else if (compositionString == "parsec-canneal-simsmall-3")
		requirement = 4;
	else if (compositionString == "parsec-canneal-simsmall-4")
		requirement = 5;
	else if (compositionString == "parsec-canneal-simsmall-5")
		requirement = 6;
	else if (compositionString == "parsec-canneal-simsmall-6")
		requirement = 7;
	else if (compositionString == "parsec-canneal-simsmall-7")
		requirement = 8;
	else if (compositionString == "parsec-canneal-simsmall-8")
		requirement = 9;
	else if (compositionString == "parsec-canneal-simsmall-9")
		requirement = 10;
	else if (compositionString == "parsec-canneal-simsmall-10")
		requirement = 11;
	else if (compositionString == "parsec-canneal-simsmall-11")
		requirement = 12;
	else if (compositionString == "parsec-canneal-simsmall-12")
		requirement = 13;
	else if (compositionString == "parsec-canneal-simsmall-13")
		requirement = 14;
	else if (compositionString == "parsec-canneal-simsmall-14")
		requirement = 15;
	else if (compositionString == "parsec-canneal-simsmall-15")
		requirement = 16;

	else if (compositionString == "parsec-ferret-simsmall-1")
		requirement = 7;
	else if (compositionString == "parsec-ferret-simsmall-2")
		requirement = 11;
	else if (compositionString == "parsec-ferret-simsmall-3")
		requirement = 15;

	else if (compositionString == "parsec-fluidanimate-simsmall-1")
		requirement = 2;
	else if (compositionString == "parsec-fluidanimate-simsmall-2")
		requirement = 3;
	else if (compositionString == "parsec-fluidanimate-simsmall-4")
		requirement = 5;
	else if (compositionString == "parsec-fluidanimate-simsmall-8")
		requirement = 9;


	else if (compositionString == "parsec-streamcluster-simsmall-1")
		requirement = 2;
	else if (compositionString == "parsec-streamcluster-simsmall-2")
		requirement = 3;
	else if (compositionString == "parsec-streamcluster-simsmall-3")
		requirement = 4;
	else if (compositionString == "parsec-streamcluster-simsmall-4")
		requirement = 5;
	else if (compositionString == "parsec-streamcluster-simsmall-5")
		requirement = 6;
	else if (compositionString == "parsec-streamcluster-simsmall-6")
		requirement = 7;
	else if (compositionString == "parsec-streamcluster-simsmall-7")
		requirement = 8;
	else if (compositionString == "parsec-streamcluster-simsmall-8")
		requirement = 9;
	else if (compositionString == "parsec-streamcluster-simsmall-9")
		requirement = 10;
	else if (compositionString == "parsec-streamcluster-simsmall-10")
		requirement = 11;
	else if (compositionString == "parsec-streamcluster-simsmall-11")
		requirement = 12;
	else if (compositionString == "parsec-streamcluster-simsmall-12")
		requirement = 13;
	else if (compositionString == "parsec-streamcluster-simsmall-13")
		requirement = 14;
	else if (compositionString == "parsec-streamcluster-simsmall-14")
		requirement = 15;
	else if (compositionString == "parsec-streamcluster-simsmall-15")
		requirement = 16;




	else if (compositionString == "parsec-swaptions-simsmall-1")
		requirement = 2;
	else if (compositionString == "parsec-swaptions-simsmall-2")
		requirement = 3;
	else if (compositionString == "parsec-swaptions-simsmall-3")
		requirement = 4;
	else if (compositionString == "parsec-swaptions-simsmall-4")
		requirement = 5;
	else if (compositionString == "parsec-swaptions-simsmall-5")
		requirement = 6;
	else if (compositionString == "parsec-swaptions-simsmall-6")
		requirement = 7;
	else if (compositionString == "parsec-swaptions-simsmall-7")
		requirement = 8;
	else if (compositionString == "parsec-swaptions-simsmall-8")
		requirement = 9;
	else if (compositionString == "parsec-swaptions-simsmall-9")
		requirement = 10;
	else if (compositionString == "parsec-swaptions-simsmall-10")
		requirement = 11;	
	else if (compositionString == "parsec-swaptions-simsmall-11")
		requirement = 12;
	else if (compositionString == "parsec-swaptions-simsmall-12")
		requirement = 13;
	else if (compositionString == "parsec-swaptions-simsmall-13")
		requirement = 14;			
	else if (compositionString == "parsec-swaptions-simsmall-14")
		requirement = 15;
	else if (compositionString == "parsec-swaptions-simsmall-15")
		requirement = 16;


	else if (compositionString == "parsec-x264-simsmall-1" || compositionString == "parsec-x264-test-1")
		requirement = 1;
	else if (compositionString == "parsec-x264-simsmall-2")
		requirement = 3;
	else if (compositionString == "parsec-x264-simsmall-3")
		requirement = 4;

	else if (compositionString == "parsec-dedup-simsmall-1")
		requirement = 4;
	else if (compositionString == "parsec-dedup-simsmall-2")
		requirement = 7;
	else if (compositionString == "parsec-dedup-simsmall-3")
		requirement = 10;
	else if (compositionString == "parsec-dedup-simsmall-4")
		requirement = 13;
	else if (compositionString == "parsec-dedup-simsmall-5")
		requirement = 16;



	
	else {
		cout <<"\n[Scheduler] [Error]: Can't find core requirement of " << compositionString <<". Please add the profile." << endl;		
		exit (1);
	}

	if (requirement > numberOfCores) {
	
		cout <<"\n[Scheduler] [Error]: Task "  << compositionString << " Core Requirement " << requirement << " is greater than number of cores " << numberOfCores << endl;		
		exit (1);

	}

	return requirement;
	
}




/** periodic
    This function is called periodically by Sniper at Interval of 100ns.
*/
void SchedulerOpen::periodic(SubsecondTime time) {
	if (time.getNS () % 1000000 == 0) { //Error Checking at every 1ms. Can be faster but will have overhead in simulation time.
		cout << "\n[Scheduler]: Time " << time.getNS () << " ns" << " [Active Tasks =  " << numberOfActiveTasks () << " | Completed Tasks = " <<  numberOfTasksCompleted () << " | Queued Tasks = "  << numberOfTasksInQueue () << " | Non-Queued Tasks  = " <<  numberOfTasksWaitingToSchedule () <<  " | Free Cores = " << numberOfFreeCores () << " | Active Tasks Requirements = " << totalCoreRequirementsOfActiveTasks () << " ] \n"<<endl;

		//Following error checking code make sure that system state is not mixed up.

		if (numberOfCores - totalCoreRequirementsOfActiveTasks () != numberOfFreeCores ()) {
			cout <<"\n[Scheduler] [Error]: Number of Free Cores + Number of Active Tasks Requirements != Number Of Cores.\n";		
			exit (1);
		}

		if (numberOfActiveTasks () + numberOfTasksCompleted () + numberOfTasksInQueue () + numberOfTasksWaitingToSchedule () != numberOfTasks) {

			cout <<"\n[Scheduler] [Error]: Task State Does Not Match.\n";		
			exit (1);
		}
	}

	if (time.getNS () % schedulingEpoch == 0) {
		
		cout << "\n[Scheduler]: Scheduler Invoked at " << time.getNS () << " ns"<<"\n"<<endl;

		fetchTasksIntoQueue (time);
				


		while (	numberOfTasksInQueue () != 0) {	
			if (!schedule (taskFrontOfQueue (), false,time)) break; //Scheduler can't map the task in front of queue.
		}

		cout << "[Scheduler]: Current mapping:" << endl;

		for (int y = 0; y < coreColumns; y++) {
			for (int x = 0; x < coreRows; x++) {
				if (x > 0) {
					cout << " ";
				}
				int coreId = getCoreNb(y, x);
				if (!isAssignedToTask(coreId)) {
					cout << " .";
				} else {
					cout << setw(2) << systemCores[coreId].assignedTaskID;
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


