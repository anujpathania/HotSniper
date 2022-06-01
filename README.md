### Next Release: CoMeT Simulator

CoMeT: CoMeT is next-generation open-source EDA toolchain for integrated core-memory interval thermal simulations of 2D, 2.5, and 3D multi-/many-core processors. CoMeT *subsumes* the code of HotSniper.

[Download CoMeT](https://github.com/marg-tools/CoMeT) 

# HotSniper 7

An EDA toolchain for interval thermal simulations of 2D multi-/many-cores in an open system.

Note: If you have a problem, please first browse the closed issues here - https://github.com/anujpathania/HotSniper/issues. If your problem continues to remain unresolved, please feel free to contact us by raising a new issue. Also, please do not forget to close the issue once we have addressed your problem. We prefer not to resolve issues over e-mail.

## Publication

### HotSniper: Sniper-Based Toolchain for Many-Core Thermal Simulations in Open Systems

Details of HotSniper can be found in our ESL 2018 paper, and please consider citing this paper in your work if you find this tool useful in your research.

> Pathania, Anuj, and Jörg Henkel. **"HotSniper: Sniper-Based Toolchain for Many-Core Thermal Simulations in Open Systems."** *IEEE Embedded Systems Letters* 11.2 (2018): 54-57.

[IEEE Xplore](https://ieeexplore.ieee.org/abstract/document/8444047) 

## The HotSniper User Manual

Please refer to [Hot Sniper User Manual](https://github.com/anujpathania/HotSniper/blob/master/The%20HotSniper%20User%20Manual.pdf) to learn how to write custom scheduling policies that perform thermal-aware Dynamic Voltage Frequency Scaling (DVFS), Task Mapping, and Task Migration.


## 1- Requirements
### Docker
HotSniper7 compiles and runs inside a Docker container. Therefore, you need to download & install Docker. 
For more info: https://docs.docker.com/engine/install/ubuntu/

After installing Docker, make sure you are able to run it without needing sudo by following instructions here - https://docs.docker.com/engine/install/linux-postinstall/

### PinPlay 
Download and extract Pinplay 3.2 to the root HotSniper7 directory as ```pin_kit```
```sh
wget --user-agent="Mozilla"  https://www.intel.com/content/dam/develop/external/us/en/protected/pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux.tar.gz
tar xf pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux.tar.gz
mv pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux pin_kit
```

## 2- Compiling HotSniper7
At this stage, the root HotSniper7 directory has a folder named ```pin_kit``` containing the PinPlay-3.2 library and a folder named ```hotspot```containing the HotSpot simulator. Since you now have Docker installed, let's create a ```container``` using the shipped ```Dockerfile```.
```sh
cd docker
make
make run
```
Now that we are inside our container, we can build HotSniper 7 and its requirements:
```sh
cd ..
```

### HotSpot
The [HotSpot] simulator is shipped with HotSniper7. All you need to do is to compile it:
```sh
cd hotspot
make
cd ..
```

### HotSniper 7
```sh
make
```

## 3- Compiling the Benchmarks

Run inside container:
```sh
#setting $GRAPHITE_ROOT to HotSniper7's root directory
export GRAPHITE_ROOT=$(pwd)
cd benchmarks
#setting $BENCHMARKS_ROOT to the benchmarks directory
export BENCHMARKS_ROOT=$(pwd)
#compiling the benchmarks
make
```


## 4- Running the Simulations
HotSniper7 is shipped with a ```simulationcontrol``` script that you can use to run batch simulations.
Run inside container:
```sh
cd simulationcontrol
PYTHONIOENCODING="UTF-8" python3 run.py
```
The path of the results' directory can be set inside the ```simulationcontrol/config.py``` file.


## 5- Evaluate your results
Quickly list the finished simulations:
```sh
cd simulationcontrol
PYTHONIOENCODING="UTF-8" python3 parse_results.py
```

Each run is stored in a separate directory in the results directory (see 4).
For quick visual check, many plots are automatically generated for you (IPS, power, etc).

To do your own (automated) evaluations, see the `simulationcontrol.resultlib` package for a set of helper functions to parse the results. See the source code of `parse_results.py` for a few examples.


## Configuration Checklist

- [ ] select technology node
  - `config/base.cfg`: `power/technology_node`
- [ ] V/f-levels
  - check `scripts/energystats.py`: `build_dvfs_table` (keep in mind that V/f-levels are specified at 22nm)
- [ ] power scaling (if technology node < 22nm)
  - check `tools/mcpat.py`: `scale_power`
- [ ] select high-level architecture
  - `simulationcontrol/config.py`: `SNIPER_CONFIG` and `NUMBER_CORES`
- [ ] set architectural parameters
  - `config/base.cfg` and other config files as specified in the previous step
- [ ] set scheduling and DVFS parameters
  - `config/base.cfg`: `scheduler/open/*` and `scheduler/open/dvfs/*`
- [ ] set `perf_model/core/frequency`
- [ ] start trial run to extract estimations from McPAT
  - start a simulation based on `simulationcontrol/run.py`: `test_static_power`, kill it after ~5ms simulated time
  - extract static power at low/high V/f levels from the command line output: take power of last / second-to-last core
  - extract area of a core from `benchmarks/energystats-temp.txt`: take processor area (including L3 cache etc.), divide by number of cores, and scale it to your technology node. If file is empty, start simulation again, kill it, and check again.
- [ ] configure static power consumption
  - `config/base.cfg`: `power/*`
  - `inactive_power` must be set to static power consumption at min V/f level
- [ ] create floorplan (`*.flp`) and corresponding thermal model (`*.bin`)
  - Option 1: use your own floorplan
    - use [MatEX] to create the thermal model (`-eigen_out`) from your floorplan
  - Option 2: use [thermallib] to create a simple regular floorplan (only per-core temperature, no finer granularity) and the corresponding thermal model
    - core width is `sqrt(core area)` from McPAT area estimations
    - example: `python3 create_thermal_model.py --amb 45 --crit 80 --core_naming sniper --core_width [core width] model [cores]x[cores]`
    - NOTE: McPAT area estimations are high, i.e., observed temperatures are too low. Therefore, using a smaller core size should be considered as an option.
- [ ] specify floorplan, thermal model and other thermal settings in config
  - `config/base.cfg`: `periodic_thermal`
  - `tdp` is defined by the floorplan, temperature limits and cooling parameters
- [ ] create your scenarios
  - `simulationcontrol/run.py` (e.g., similar to `def example`)
- [ ] set your output folder for traces
  - `simulationcontrol/config.py`: `RESULTS_FOLDER`
  - This folder usually is outside of the HotSniper folder because we don't want to commit results (large files) to the simulator repo.
- [ ] verify all configurations in `sim.cfg` of a finished run


## HOWTO

### A- Implement your own mapping / DVFS policy
These policies are implemented in `common/scheduler/policies`.
Mapping policies derive from `MappingPolicy`, DVFS policies derive from `DVFSPolicy`.
After implementing your policy, instantiate it in `SchedulerOpen::initMappingPolicy` / `SchedulerOpen::initDVFSPolicy`.


## Common Errors
```UnicodeEncodeError: 'ascii' codec can't encode character '\xb0' in position 61: ordinal not in range(128)```
```sh
export PYTHONIOENCODING="UTF-8"
```

## Code Acknowledgements

  Sniper: <http://snipersim.org>
  
  McPat: https://www.hpl.hp.com/research/mcpat/
  
  HotSpot: <http://lava.cs.virginia.edu/HotSpot/>
  
  MatEx: http://ces.itec.kit.edu/846.php
  
  thermallib: https://github.com/ma-rapp/thermallib

