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

* Step 3: Compile Benchmarks (Downgrading Perl removes PARSEC compilations error - https://groups.google.com/forum/#!msg/snipersim/LF_VfebuLSI/AVdiq4y0hk8J). Working benchmarks with sim-small input option: blackscholes, bodytrack, canneal, dedup, facesim, ferret, fluidanimate, raytrace, streamcluster,swaptions, x264

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



# Feature 2: Periodic Power Tracing 

* Major Files of Interest "tools/mcpat.py", "scripts/periodic-power.py"

* This allows for you to obtain a power trace giving power of different components at a customizable intervals. You can add or remove elements in power trace by modifying "tools/mcpat.py".

* Currently following elements are tracked for each core -

  L2 - Private L2

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

* You can toggle  tracking of any of the above indiviusual elements by modifying in "[periodic_power]" property "base.cfg". By default all elements tracking is set to true.

* Example to run Perioidic Power Tracing at 10000 ns intervals -

$ ./run-sniper -n 2 -c gainestown  --benchmarks=parsec-blackscholes-test-1 --no-roi --sim-end=last -senergystats -speriodic-power:10000

* The dump would be created in benchmarks folder in file with name - "PeriodicPower.log"

# Hotspot Integration (Expected Soon).
