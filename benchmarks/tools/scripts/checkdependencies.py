#!/usr/bin/env python

import sys, os

# list of (packagename, filename)

DEPENDENCIES = [
  ('g++ / gcc-g++', '/usr/bin/g++'),
  ('gfortran', '/usr/bin/f95'),
  ('m4', '/usr/bin/m4'),
  ('wget', '/usr/bin/wget'),
]

ALTERNATIVES = [
  ('/usr/lib', '/usr/lib/x86_64-linux-gnu'),
  ('/usr/lib', '/usr/lib64'),
]

missing = []

def find_file(filename):
  if os.path.exists(filename):
    return True
  for pattern, replacement in ALTERNATIVES:
    if os.path.exists(filename.replace(pattern, replacement)):
      return True
  return False

for package, filename in DEPENDENCIES:
  if not find_file(filename):
    missing.append(package)

if missing:
  print >> sys.stderr, '*** Please install the following package%s: %s' % \
                       (len(missing)>1 and 's' or '', ' '.join(missing))
  sys.exit(1)
else:
  sys.exit(0)
