#!/bin/bash

export PYTHONIOENCODING="UTF-8"

cd ../
export GRAPHITE_ROOT="$(pwd)"
export BENCHMARKS_ROOT="$GRAPHITE_ROOT/benchmarks"
make
