#!/usr/bin/python3

import re
import sys

# Read hotspot floorplan from stdin and offset it by `offset_x` and `offset_y`

if len(sys.argv) != 3:
    print('usage: {} offset_x offset_y < floorplan.flp'.format(sys.argv[0]))
    sys.exit(1)

offset_x = float(sys.argv[1])
offset_y = float(sys.argv[2])
for line in sys.stdin:
    if line.startswith("#"):
        print(line, end='')
        continue
    component = line.split()[0]
    dimensions = line.split()[1:]
    dimensions = list(map(float, dimensions))
    dimensions[2] = dimensions[2] + offset_x
    dimensions[3] = dimensions[3] + offset_y
    dimensions = list(map(lambda x: f"{x:.5g}", dimensions))
    print(component + '\t' + '\t'.join(dimensions))
