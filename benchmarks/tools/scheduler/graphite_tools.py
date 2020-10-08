import sys, os
sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'python'))
import env_setup_bm

sniper_path = env_setup_bm.sim_root()
if sniper_path:
  sys.path.append(os.path.join(sniper_path, 'tools'))
  try:
    from sniper_lib import *
  except ImportError:
    sys.stderr.write('Cannot find sniper_lib. Make sure SNIPER_ROOT is set correctly.\n')
    sys.exit(1)
else:
  sys.stderr.write('Cannot find sniper_lib. Make sure SNIPER_ROOT is set correctly.\n')
  sys.exit(1)
