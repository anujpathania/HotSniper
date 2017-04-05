# SniperPlus
An Enhanced Research Version of Sniper MultiCore Simulator (http://snipersim.org).

Sniper Version used as base - Sniper 6.1

For Queries: anujpathania@protonmail.com

# Build Instructions (Tested on Ubuntu 16.04)

Step 1: Go into the Sniper Code Directory

$ cd SniperPlus

Step 2: Sucessfully Compile Sniper. You will have to install PIN and several other repositories to make this work. If you face a problem, email me only if it the problem present in the SniperPlus and not in original Sniper 6.1. Works best with pin-2.14-71313, gcc-4.8 and g++-4.8.

$ make

Step 3: Make sure Sniper is working properly.

$ cd SniperPlus/test/fft

Step 4: Compile Benchmarks (Downgrading Perl removes PARSEC compilations error - https://groups.google.com/forum/#!msg/snipersim/LF_VfebuLSI/AVdiq4y0hk8J). Working benchmarks with sim-small input option: blackscholes, bodytrack, canneal, dedup, facesim, ferret, fluidanimate, raytrace, streamcluster,swaptions, x264

$ wget http://snipersim.org/packages/sniper-benchmarks.tbz
$ tar xjf sniper-benchmarks.tbz
$ cd benchmarks
$ export SNIPER_ROOT=/path/to/sniper
$ make - j 4



# Feature Enhancements
