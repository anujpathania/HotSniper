#!/usr/bin/python3

import re
import sys

# Read hotspot floorplan from stdin and scale it down by `down_scale_factor`

if len(sys.argv) != 2:
    print('usage: {} downscale_factor < floorplan.flp'.format(sys.argv[0]))
    sys.exit(1)

down_scale_factor = float(sys.argv[1])
for line in sys.stdin:
    component = line.split('\t')[0]
    dimensions = re.findall(r'\b\d+\.\d+', line)
    scaled_dimensions = list(map(lambda x: str(float(x) / down_scale_factor), dimensions))
    print(component + '\t' + '\t'.join(scaled_dimensions))
