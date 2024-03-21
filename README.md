### Also See: CoMeT Simulator

CoMeT: CoMeT is next-generation open-source EDA toolchain for integrated core-memory interval thermal simulations of 2D, 2.5, and 3D multi-/many-core processors. CoMeT (partially) *subsumes* the code of HotSniper.

[Download CoMeT](https://github.com/marg-tools/CoMeT)

# HotSniper

An EDA toolchain for interval thermal simulations of 2D multi-/many-cores in an open system.

## Publication

### HotSniper: Sniper-Based Toolchain for Many-Core Thermal Simulations in Open Systems

Details of HotSniper can be found in our ESL 2018 paper, and please consider citing this paper in your work if you find this tool useful in your research.

> Pathania, Anuj, and Jörg Henkel. **"HotSniper: Sniper-Based Toolchain for Many-Core Thermal Simulations in Open Systems."** *IEEE Embedded Systems Letters* 11.2 (2018): 54-57.

[IEEE Xplore](https://ieeexplore.ieee.org/abstract/document/8444047)

## The HotSniper User Manual

Please refer to [Hot Sniper User Manual](https://github.com/anujpathania/HotSniper/blob/master/The%20HotSniper%20User%20Manual.pdf) to learn how to write custom scheduling policies that perform thermal-aware Dynamic Voltage Frequency Scaling (DVFS), Task Mapping, and Task Migration.

## Ground Rules

Found a Bug, Report [Here](https://github.com/anujpathania/HotSniper/issues)! Have a Question, Ask [Here](https://github.com/anujpathania/HotSniper/discussions)!

**No Direct Emails.**

## 1- Requirements
### Docker
HotSniper compiles and runs inside a Docker container. Therefore, you need to download & install Docker.
For more info: https://docs.docker.com/engine/install/ubuntu/

After installing Docker, make sure you are able to run it without needing sudo by following instructions here - https://docs.docker.com/engine/install/linux-postinstall/

### PinPlay
Extract Pinplay 3.2 to the root HotSniper directory as ```pin_kit```
```sh
tar xf pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux.tar.gz
mv pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux pin_kit
```


## 2- Compiling HotSniper
At this stage, the root HotSniper directory has a folder named ```pin_kit``` containing the PinPlay-3.2 library and a folder named ```hotspot```containing the HotSpot simulator. Since you now have Docker installed, let's create a ```container``` using the shipped ```Dockerfile```.
```sh
cd docker
sudo apt install make
make
make run
```
Now that we are inside our container, we can build HotSniper and its requirements:
```sh
cd ..
```

### HotSpot
The [HotSpot] simulator is shipped with HotSniper. All you need to do is to compile it:
```sh
cd hotspot
make
cd ..
```

### HotSniper
```sh
make
```

## 3- Compiling the Benchmarks

Run inside container:
```sh
#setting $GRAPHITE_ROOT to HotSniper's root directory
export GRAPHITE_ROOT=$(pwd)
cd benchmarks
#setting $BENCHMARKS_ROOT to the benchmarks directory
export BENCHMARKS_ROOT=$(pwd)
#compiling the benchmarks
make
cd ..
```


## 4- Running the Simulations
HotSniper is shipped with a ```simulationcontrol``` script that you can use to run batch simulations.
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

- [ ] select technology node (22nm or larger)
  - `config/base.cfg`: `power/technology_node`
- [ ] V/f-levels
  - check `scripts/energystats.py`: `build_dvfs_table` (keep in mind that V/f-levels are specified at 22nm)
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
- [ ] specify the floorplan with the `floorplan` parameter, the corresponding the thermal model with the `hotspot_config` parameter and other thermal settings in `config`
  - `config/base.cfg`: `periodic_thermal`
  - `tdp` is defined by the floorplan, temperature limits and cooling parameters.
  - make sure that the `perf_model/cache/levels` is set to 3 if the floorplan has a L3 cache and it set to 2 if it does not.
  - The `hotspot` directory contains floorplans and corresponding hotspot configurations for a four core, a sixteen core and a sixty-four core gainestown processor.
- [ ] To create a new floorplan use the `create` script from the `floorplanlib` directory. For example to create a sixteen core gainestown floorplan run this command outside the docker environment:
  - `./create.py --cores 4x4 --subcore-template gainestown_core.flp --out gainestown_4x4`
  - Copy the generated floorplan `gainestown_4x4.flp` and the hotspot config file `gainestown_4x4.hotspot_config` from the generated `gainestown_4x4` directory to the `hotspot` directory. And then set the configuration parameters `floorplan` and `hotspot_config` in `base.cfg` to point to these new floorplan and hotspot configuration files.
  - When you change the number of cores you will also need to update the `NUMBER_CORES` as was mentioned above.
  - For larger floorplans we recommend changing the `-model_type` to `grid` in the hotspot configuration file to speed the thermals calculation.
- [ ] To get track the wearout of the components enable the reliability modeling in the `reliability` section.
- [ ] create your scenarios
  - `simulationcontrol/run.py` (e.g., similar to `def example`)
- [ ] set your output folder for traces
  - `simulationcontrol/config.py`: `RESULTS_FOLDER`
  - This folder usually is outside of the HotSniper folder because we don't want to commit results (large files) to the simulator repo.
- [ ] verify all configurations in `sim.cfg` of a finished run

## Using Heartbeat Functionality
HotSniper supports program performance monitoring using the *Heartbeat Framework*. Several PARSEC programs are supported out of the box, which are: blackscholes, bodytrack, canneal, dedup, fluidanimate, streamcluster, swaptions and x264.

Enabling heartbeat functionality:
- Set `simulationcontrol/config.py::ENABLE_HEARTBEATS` variable to `True`
- Add the "hb_enabled" string to the `base_configuration` argument of `simulationcontrol/run.py::run()` function call.

> The "simulationcontrol/run.py::run_multi()" function serves as a template for the second step.

With these two parameters set, the simulation will start with Heartbeat functionality enabled, resulting in the collection of heartbeat data files for each program, identified by the program app ids. A simulation running one program will result in the "0.hb.log" file, accompanied with the "0.hb.png" and "0.hb.histogram.png" visualizations.

## Common Errors
```UnicodeEncodeError: 'ascii' codec can't encode character '\xb0' in position 61: ordinal not in range(128)```
```sh
export PYTHONIOENCODING="UTF-8"
```

## Code Acknowledgements

  Sniper: <http://snipersim.org>

  McPat: https://www.hpl.hp.com/research/mcpat/

  HotSpot: <http://lava.cs.virginia.edu/HotSpot/>

  HeartBeats: "https://github.com/libheartbeats/heartbeats"

