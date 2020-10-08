#!/usr/bin/env python
# Create the scripts that define linker flags needed for running an app under Graphite.

import os

flags = [
  ('HOOKS_DIR', '${BENCHMARKS_ROOT}/tools/hooks'),
  ('HOOKS_CC', 'gcc'),
  ('HOOKS_CXX', 'g++'),
  ('HOOKS_FC', 'f95'),
  ('LD', 'g++'),
  ('HOOKS_CFLAGS', '-I${HOOKS_DIR} -I${GRAPHITE_ROOT}/include'),
  ('HOOKS_CXXFLAGS', '${HOOKS_CFLAGS}'),
  ('HOOKS_LDFLAGS', '-uparmacs_roi_end -uparmacs_roi_start -L${HOOKS_DIR} -lhooks_base -lrt -pthread'),
  ('HOOKS_LDFLAGS_NOROI', '-uparmacs_roi_end -uparmacs_roi_start -L${HOOKS_DIR} -lhooks_base_noroi -lrt -pthread'),
  ('HOOKS_LDFLAGS_DYN', '-uparmacs_roi_end -uparmacs_roi_start -L${HOOKS_DIR} -lhooks_base -lrt -pthread'),
  ('HOOKS_LD_LIBRARY_PATH', ''),
]

message = '# This file is auto-generated, changes made to it will be lost. Please edit %s instead.' % os.path.basename(__file__)

file('buildconf.sh', 'w').write('\n'.join(
  [ message, '' ] +
  [ '%s="%s"' % i for i in flags ]
) + '\n')

file('buildconf.makefile', 'w').write('\n'.join(
  [ message, '' ] +
  [ '%s:=%s' % i for i in flags ]
) + '\n')

file('buildconf.py', 'w').write('\n'.join(
  [ message, '' ] +
  [ str(dict(flags)) ]
) + '\n')
