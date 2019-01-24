# HotSniper
An enchanced multi/many-core simulator derived from Sniper MultiCore Simulator (http://snipersim.org).

Sniper Version used as base - Sniper 6.1

Creators: Martin Rapp, Anuj Pathania

License: MIT

For Queries: martin.rapp@kit.edu, pathania@comp.nus.edu.sg

Details of HotSniper can be found in paper "HotSniper: Sniper-Based Toolchain for Many-Core Thermal Simulations in Open Systems" published in Embedded Systems Letter (ESL), and please consider citing this paper in your work if you find this tool useful in your research.


# Build Instructions (Tested on Ubuntu 16.04.4)

If you experience any issues, chances are that you are not the first one. We collected some common issues and solutions at the bottom of this README.

* Install a Supported Kernel Version

$ sudo apt-get install 4.4.0-127-generic

Reboot and Select in Grub Bootloader "Advanced options for Ubuntu" >> "Ubuntu, with Linux 4.4.0-127-generic"

* Install Supported gcc, g++ and perl Versions

$ sudo apt-get install gcc-4.8 g++-4.8 perlbrew

$ sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 100

$ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 100

* Install Sniper

$ sudo apt-get install zlib1g-dev libbz2-dev libboost-dev libsqlite3-dev

$ cd HotSniper/HotSniper

$ wget https://software.intel.com/sites/landingpage/pintool/downloads/pin-2.14-71313-gcc.4.4.7-linux.tar.gz

$ tar -xvzf pin-2.14-71313-gcc.4.4.7-linux.tar.gz

$ mv pin-2.14-71313-gcc.4.4.7-linux pin_kit

$ rm pin-2.14-71313-gcc.4.4.7-linux.tar.gz

$ make

* Install PARSEC (and Other) Benchmark

$ tar -xvzf benchmarks.tar.gz

$ cd benchmarks

$ sudo apt-get install gfortran m4 xsltproc libx11-dev libxext-dev libxt-dev libxmu-dev libxi-dev

$ make -j 4


* Install Hotspot

$ cd ..

$ tar -xvzf hotspot.tar.gz

$ cd hotspot

$ make

* Test PARSEC Multi-Program Execution Simulation

$ cd ../benchmarks

$ ./run-sniper -n 64 -c gainestown --benchmarks=parsec-blackscholes-test-1,parsec-bodytrack-test-1 --no-roi --sim-end=last

* Test Thermal Simulation

$ ./run-sniper -n 2 -c gainestown --benchmarks=parsec-blackscholes-test-1 --no-roi --sim-end=last -senergystats:100000 -speriodic-power:100000


# Feature 1: Open Scheduler 

* Major Files of Interest "scheduler_open.cc", "scheduler_open.h", "base.cfg", "policies/*"

* This scheduler allows support for open system workloads. Scheduler can be set in "base.cfg" along with configuration parameters. 

* To switch to open scheduler, set -

	[scheduler]
	type = open

* Set the mapping algorithm to be used -
	[scheduler/open]
	logic = first_unused

  Supported value: "first_unused" (maps the threads to first free cores available searched linearly).

* Set the granularity at which the open system workload queue is checked -

	[scheduler/open]
	epoch = 10000000

* Set the queueing policy -
	
	[scheduler/open]
	queue = FIFO

  Supported value: "FIFO" (First-In-First-Out).

* Set the distribution of workload arrival -

	[scheduler/open]
	queue = poisson

   Supported value: "uniform" (uniform distribution) and "poisson" (Poisson distribution).

* Note that Open Scheduler will only work with multi-program mode i.e. with option --benchmarks

* Note for Open Scheduler to work it requires to know beforehand, in worst case, how many threads the benchmark would produce as it only supports one thread per core execution model. This needs to be profiled (use static scheduler). For many benchmark+input this is done in function "coreRequirementTranslation" in file "scheduler_open.cc". Add more enteries if you want to support them.

* Set the DVFS algorithm to be used -
	[scheduler/open/dvfs]
	logic = off

* Set min and max DVFS frequencies, as well as DVFS step size
	[scheduler/open/dvfs]
	min_frequency = 1.0
	max_frequency = 2.4
	frequency_step_size = 0.1

* Note: this frequency range should match with the many-cores base frequency set in 
	[perf_model/core]
	frequency = 2.4

* Set DVFS epoch
	[scheduler/open/dvfs]
	dvfs_epoch = 1000000

* Note: If DVFS is used, this value should be a multiple (or the same as) the period used for periodic power tracing (see Feature #2).



# Feature 2: Periodic Power (using McPAT) & Temperature Tracing (using HotSpot)

* Major Files of Interest "tools/mcpat.py", "scripts/periodic-power.py"

* This allows for you to obtain a power and thermal trace giving power of different components at a customizable intervals. You can add or remove elements in power trace by modifying "tools/mcpat.py".

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

* You need to add the correct floorplan (including all the power values you are tracking) to Hotspot folder. For example see "/HotSniper/hotspot/gainestown_2.flp" that comes with default. You need to mention the floorplan to be used in the config file "base.cfg".

	[periodic_thermal]
	floorplan = gainestown_2.flp

* You need to provide the granularity to run Perioidic Power Tracing in nano seconds as parameter to "-senergystats" and "-spreiodicpower" at command line.

* The power and thermal dump would be created in benchmarks folder in file with name - "PeriodicPower.log" and "PeriodicThermal.log".

* PeriodicFrequency and CPI stack traces are generated i nthe files "PeriodicFrequency.log" and "PeriodicCPIStack.log".

* The instantaneous- power and thermal value can also be read in the Sniper program code itself using "getPowerOfComponent" and "getTemperatureOfComponent" function in "performance_counters.cc", respectively. This can be used to feedback power and thermal information to your scheduler for taking decisions.

* If thermal values are not required then processing overhead due to HotSpot execution can be removed with the setting below attribute to false in "base.cfg".

	[periodic_thermal]
	enabled = true


# Future/Ongoing Feature Integrations

* Integration of HeartBeat API from MIT; refer "Application Heartbeats for Software Performance and Health".

* Integration of Thermal Safe Power (TSP); refer "TSP: Thermal Safe Power - Efficient Power Budgeting for Many-Core Systems in Dark Silicon".

* Integration of Reliability Models.

* Merge with Sniper 8


# Problems and Solutions

## The C++ ABI of your compiler does not match the ABI of the pin kit. This kit requires gcc 3.4 or later

Seen with gcc/g++ 5. Use gcc 4.8 and g++ 4.8 instead.

## Source/pin/injector_nonmac/auxvector.cpp: CopyAux: 291: unexpected AUX VEC type 26

Seen with kernel versions > 4.9. Downgrade to kernel version 4.9.
This seems to be a limitation of the Intel VTune library, which is used by PIN.
