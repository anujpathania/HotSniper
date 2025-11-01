# !/bin/bash

# This script is used to rebuild the benchmarks and run the simulations.

cd ..
make

export GRAPHITE_ROOT=$(pwd)
cd benchmarks
#setting $BENCHMARKS_ROOT to the benchmarks directory
export BENCHMARKS_ROOT=$(pwd)
#compiling the benchmarks
make
cd ..

cd simulationcontrol
PYTHONIOENCODING="UTF-8" python3 run.py

