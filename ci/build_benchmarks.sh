set -e

export GRAPHITE_ROOT=$(pwd)
cd benchmarks
export BENCHMARKS_ROOT=$(pwd)
make
