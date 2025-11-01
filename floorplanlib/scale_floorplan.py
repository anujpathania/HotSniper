#!/usr/bin/python3

import re
import sys

# Read hotspot floorplan from stdin and scale it by `scale_factor`

if len(sys.argv) != 2:
    print('usage: {} scale_factor < floorplan.flp'.format(sys.argv[0]))
    sys.exit(1)

scale_factor = float(sys.argv[1])
for line in sys.stdin:
    if line.startswith("#"):
        print(line, end='')
        continue
    component = line.split()[0]
    dimensions = line.split()[1:]
    dimensions = list(map(lambda x: float(x) * scale_factor, dimensions))
    dimensions = list(map(lambda x: f"{x:.5g}", dimensions))
    print(component + '\t' + '\t'.join(dimensions))
