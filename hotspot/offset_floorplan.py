#!/usr/bin/python3

import re
import sys

# Read hotspot floorplan from stdin and offset it down by `offset_x` and `offset_y`

if len(sys.argv) != 3:
    print('usage: {} offset_x offset_y < floorplan.flp'.format(sys.argv[0]))
    sys.exit(1)

offset_x = float(sys.argv[1])
offset_y = float(sys.argv[2])
for line in sys.stdin:
    component = line.split('\t')[0]
    dimensions = re.findall(r'\b\d+\.\d+', line)
    #dimensions = list(map(lambda x: str(float(x) / down_scale_factor), dimensions))
    dimensions[2] = str(float(dimensions[2]) + offset_x)
    dimensions[3] = str(float(dimensions[3]) + offset_y)
    print(component + '\t' + '\t'.join(dimensions))
