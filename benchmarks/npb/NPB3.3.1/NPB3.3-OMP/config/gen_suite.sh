#!/bin/bash

BENCHMARKS="ft mg sp lu bt is ep cg ua dc"
INPUTS="S W A B C D E"

(
	for bm in $BENCHMARKS
	do
		for input in $INPUTS
		do
			echo -e $bm\\t$input
		done
	done
) > suite.def
