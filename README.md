# SniperPlus
An enchanced multi/many-core simulator derived from Sniper MultiCore Simulator (http://snipersim.org).

Sniper Version used as base - Sniper 6.1

Creator: Anuj Pathania

License: MIT

For Queries: anuj.pathania@kit.edu

List of our publications based on this tool -

1. [TACO' 17] Defragmentation of Tasks in Many-Core Architecture.



# Build Instructions (Tested on Ubuntu 16.04)

* Step 1: Go into the Sniper Code Directory

$ cd SniperPlus

* Step 2: Sucessfully Compile Sniper. You will have to install PIN and several other repositories to make this work. If you face a problem, email me only if it the problem present in the SniperPlus and not in original Sniper 6.1. Works best with pin-2.14-71313, gcc-4.8 and g++-4.8.

$ make

* Step 3: Compile Benchmarks (Downgrading Perl removes PARSEC compilations error - https://groups.google.com/forum/#!msg/snipersim/LF_VfebuLSI/AVdiq4y0hk8J). Working benchmarks with sim-small input option: blackscholes, bodytrack, canneal, dedup, facesim, ferret, fluidanimate, raytrace, streamcluster,swaptions, x264. Use the version given with SniperPlus not original Sniper.

$ tar -xvzf benchmarks.tar.gz

$ cd benchmarks

$ export SNIPER_ROOT=/path/to/sniper

$ make - j 4

* Step 4: Test PARSEC benchmarks in multi-program mode.

$ ./run-sniper -n 64 -c gainestown --benchmarks=parsec-blackscholes-test-1,parsec-bodytrack-test-1 --no-roi --sim-end=last





# Feature 1: Open Scheduler 

* Major Files of Interest "scheduler_open.cc", "scheduler_open.h", "base.cfg"

* This scheduler allows support for open system workloads. Scheduler can be set in "base.cfg" along with configuration parameters. 

* To switch to open scheduler, set -

	[scheduler]
	type = open

* Set the scheduling algorithm to be used -
	[scheduler/open]
	logic = default

  Supported value: "default" (maps the threads to first free cores available searched linearly).

* Set the granularity at which the open system workload queue is checked -

	[scheduler/open]
	epoch = 10000000

* Set the queueing policy -
	
	[scheduler/open]
	queue = FIFO

  Supported value: "FIFO" (First-In-First-Out).

* Set the distribution of workload arrival -

	[scheduler/open]
	queue = uniform


   Supported value: "uniform" (uniform distribution). Expect out of the box support for Poisson distribution in future.

* Note that Open Scheduler will only work with multi-program mode i.e. with option --benchmarks

* Note for Open Scheduler to work it requires to know beforehand, in worst case, how many threads the benchmark would produce as it only supports one thread per core execution model. This needs to be profiled (use static scheduler). For many benchmark+input this is done in function "coreRequirementTranslation" in file "scheduler_open.cc". Add more enteries if you want to support them.



# Feature 2: Periodic Power (using McPAT) & Temperature Tracing (using HotSpot)

* Major Files of Interest "tools/mcpat.py", "scripts/periodic-power.py"

* This allows for you to obtain a power and thermal trace giving power of different components at a customizable intervals. You can add or remove elements in power trace by modifying "tools/mcpat.py".

* Compile HotSpot (Use the one shipped with SniperPlus not the original).

$ cd SniperPlus

$ tar -xvzf hotspot.tar.gz

$ cd hotspot

$ make

$ cd ../benchmarks

* Currently following elements are tracked for each core. You can toggle  tracking of any of the above indiviusual elements by modifying in "[periodic_power]" property "base.cfg". By default all elements tracking is set to true.

  System Level:

  L3 - L3 Cache

  Core Level:

  L2 - L2 Cache

  IS - Instruction Scheduler

  RF - Register Files

  RBB - Result Broadcast Bus

  RU - Renaming Unit

  BP - Branch Predictor

  BTB - Branch Target Buffer

  IB - Instruction Buffer

  ID - Instruction Decoder

  IC - Instruction Cache

  DC - Data Cache

  CALU - Complex ALU

  FALU - Floating Point ALU

  IALU - Integer ALU

  LU - Load Unit

  SU - Store Unit

  MMU - Memory Management Unit

  IFU - Instruction Fetch Unit

  LSU - Load Store Unit

  EU  - Execution Unit

  TP - Total Power

* You need to add the correct floorplan (including all the power values you are tracking) to Hotspot folder. For example see "/SniperPlus/hotspot/gainestown_2.flp" that comes with default. You need to mention the floorplan to be used in the config file "base.cfg".

	[periodic_thermal]
	floorplan = gainestown_2.flp

* Example to run Perioidic Power Tracing at 10000 ns intervals -

$ ./run-sniper -n 2 -c gainestown  --benchmarks=parsec-blackscholes-test-1 --no-roi --sim-end=last -senergystats -speriodic-power:10000

* The power and thermal dump would be created in benchmarks folder in file with name - "PeriodicPower.log" and "PeriodicThermal.log"

* The instantaneous- power and thermal value can also be read in the Sniper program code itself using "getPowerOfComponent" and "getTemperatureOfComponent" function in "scheduler_open.cc", respectively. This can be used to feedback power and thermal information to your scheduler for taking decisions.
