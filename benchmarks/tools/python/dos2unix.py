#!/usr/bin/env python
"""\
convert dos linefeeds (crlf) to unix (lf)
usage: dos2unix.py <input> <output>
"""

import sys
import os

if len(sys.argv[1:]) != 2:
  sys.exit(__doc__)

content = ''
outsize = 0

with open(os.path.join(os.getcwd(), sys.argv[1]), 'rb') as infile:
  content = infile.read()
with open(os.path.join(os.getcwd(), sys.argv[2]), 'wb') as output:
  for line in content.splitlines():
    outsize += len(line) + 1
    output.write(line + b'\n')

print("Done. Stripped %s bytes." % (len(content)-outsize))
