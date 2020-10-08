import sys, os, time, getopt, subprocess, tempfile


abspath = lambda d: os.path.abspath(os.path.join(d))

HOME = abspath(os.path.dirname(__file__))
BINDIR = os.path.join(HOME, 'NPB3.3.1', 'NPB3.3-OMP', 'bin')
OPTIONS = eval(open(os.path.join(HOME, '../tools/hooks', 'buildconf.py')).read())

apps = [
  'bt',
  'cg',
  'dc',
  'ep',
  'ft',
  'is',
  'lu',
  'mg',
  'sp',
  'ua',
]

inputmap = {
  'test': 'S',
  'small': 'W',
  'large': 'A',
  'huge': 'B',
  'S': 'S',
  'W': 'W',
  'A': 'A',
  'B': 'B',
  'C': 'C',
  'Ar': 'A',
  'Br': 'B',
  'Cr': 'C',
  'D': 'D',
  'E': 'E',
}

# eb-2-02, 8 threads, icc compiled, performance, no prefetchers
runtimes = {
  'bt': {'A':    53.96564695,   'B':  214.2590554,    'C':  789.9070917},
  'cg': {'A':     3.114550899,  'B':  127.9632877,    'C':  271.068037},
  #'dc': {},
  'ep': {'A':     4.790898272,  'B':   12.72902907,   'C':   76.38720944},
  'ft': {'A':     3.62629533,   'B':   41.82870253,   'C':  254.5038425},
  'is': {'A':     1.040606558,  'B':    3.892401331,  'C':   11.20285399},
  'lu': {'A': 1362.603836,      'B': 2193.971619,     'C': 3559.771559},
  'mg': {'A':    3.010070149,   'B':   14.17050122,   'C':  138.3503323},
  'sp': {'A':   58.98577289,    'B':  229.702654,     'C': 1277.572595},
  'ua': {'A':  161.3773769,     'B':  390.0608421,    'C': 1238.653255},
}

iterations = {
  'bt': {'A': 200, 'B': 200, 'C': 200},
  'cg': {'A':  15, 'B':  75, 'C':  75},
  #'dc': {},
  'ep': {'A':   1, 'B':   1, 'C':   1},
  'ft': {'A':   6, 'B':  20, 'C':  20},
  'is': {'A':  10, 'B':  10, 'C':  10},
  'lu': {'A': 250, 'B': 250, 'C': 250},
  'mg': {'A':   4, 'B':  20, 'C':  20},
  'sp': {'A': 400, 'B': 400, 'C': 400},
  'ua': {'A': 200, 'B': 200, 'C': 200},
}

# Reduced iteration counts (warmup-start, roi-start, roi-end)
inputs_reduced = {
  'Ar': { 'bt': (1, 20, 39),                                                  'lu': (1, 3, 4),                  'sp': (1, 20, 39), 'ua': (1, 5, 35) },
  'Br': { 'bt': (1, 3, 4), 'cg': (1, 3, 4), 'ft': (1, 2, 2),                  'lu': (1, 2, 2), 'mg': (1, 3, 4), 'sp': (1, 3, 4),   'ua': (1, 5, 25) },
  'Cr': { 'bt': (1, 2, 2), 'cg': (1, 2, 2), 'ft': (1, 2, 2), 'is': (1, 3, 4), 'lu': (1, 2, 2), 'mg': (1, 2, 2), 'sp': (1, 2, 2),   'ua': (1, 2, 2) },
}

def allbenchmarks():
  return tuple(apps)

def allinputs():
  return inputmap.keys()

class Program:

  def __init__(self, program, nthreads, inputsize, benchmark_options = []):
    if program not in allbenchmarks():
      raise ValueError("Invalid benchmark %s" % program)
    if inputsize not in allinputs():
      raise ValueError("Invalid input size %s" % inputsize)
    self.program = program
    self.nthreads = int(nthreads)
    self.nthreads_force = 'force_nthreads' in benchmark_options
    self.inputsize = inputmap[inputsize]
    self.reduced = inputs_reduced.get(inputsize, {}).get(program, (None, None, None))
    self.cleanup = None
    if program == 'dc':
      self.cleanup = 'rm ADC.*'


  def get_nthreads(self):
    if int(OPTIONS.get('USE_ICC', '0')) and not self.nthreads_force:
      # ICC has a master thread plus N worker threads
      return self.nthreads - 1
    else:
      return self.nthreads


  def pgm_name(self):
    return '%s.%s.x' % (self.program, self.inputsize)

  def pgm_path(self):
    return os.path.join(BINDIR, self.pgm_name())

  def run(self, graphitecmd, postcmd = ''):
    if not os.path.exists(self.pgm_path()):
      raise ValueError("Input size %s not compiled" % self.inputsize)
    env = [
      'ulimit -s 1048576',
      'export OMP_NUM_THREADS=%d' % self.get_nthreads(),
      'export OMP_THREAD_LIMIT=%d' % self.get_nthreads(),
      'export OMP_WAIT_POLICY=passive',
    ]
    for k, v in zip(('PARMACS_ITER_WARMUP', 'PARMACS_ITER_ROIBEGIN', 'PARMACS_ITER_ROIEND'), self.reduced):
      if v is not None:
        env.append('export %s=%d' % (k, v))
    rc = run_bm(self.program, self.pgm_path(), graphitecmd, env = '; '.join(env) + '; LD_PRELOAD=%s' % os.path.join(HOME, '..', 'tools', 'hooks', 'libhooks_base_pre.so'), postcmd = postcmd)
    if self.cleanup:
      os.system(self.cleanup)
    return rc


  def rungraphiteoptions(self):
    return ''


def run(cmd):
  sys.stdout.flush()
  sys.stderr.flush()
  return os.system(cmd)

def run_bm(bm, cmd, submit, env, postcmd = ''):
  print '[NPB]', '[========== Running benchmark', bm, '==========]'
  cmd = env + ' ' + submit + ' ' + cmd + ' ' + postcmd
  print '[NPB]', 'Running \'' + cmd + '\':'
  print '[NPB]', '[---------- Beginning of output ----------]'
  rc = run(cmd)
  print '[NPB]', '[----------    End of output    ----------]'
  print '[NPB]', 'Done.'
  return rc
