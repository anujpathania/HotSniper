import sys, os, time, getopt, subprocess, tempfile
from parsec_platform import *


abspath = lambda d: os.path.abspath(os.path.join(d))

HOME = abspath(os.path.dirname(__file__))

__allbenchmarks = None

def allbenchmarks():
  global __allbenchmarks
  if not __allbenchmarks:
    try:
      benchmarks = subprocess.Popen([ '%(HOME)s/parsec-2.1/bin/parsecmgmt' % globals(), '-a', 'info' ], stdout = subprocess.PIPE).communicate()
      benchmarks = [ line[15:].split(' ')[0] for line in benchmarks[0].split('\n') if line.startswith('[PARSEC]     - ') and (line.endswith(' (apps)') or line.endswith(' (kernels)')) ]
      __allbenchmarks = sorted(benchmarks)
    except OSError:
      return None
  return __allbenchmarks


def allinputs():
  return [ f[:-8] for f in os.listdir('%(HOME)s/parsec-2.1/config' % globals()) if f.endswith('.runconf') ]



def log2(n):
  log2n = -1
  while n:
    n >>= 1
    log2n += 1
  return log2n



class Program:

  def __init__(self, program, nthreads, inputsize, benchmark_options = []):
    if program not in allbenchmarks():
      raise ValueError("Invalid benchmark %s" % program)
    if inputsize not in allinputs():
      if inputsize in ('small', 'medium', 'large'):
        inputsize = 'sim' + inputsize
      else:
        raise ValueError("Invalid input size %s" % inputsize)
    self.program = program
    self.nthreads = int(nthreads)
    self.nthreads_force = 'force_nthreads' in benchmark_options
    self.inputsize = inputsize
    if program in ('freqmine',):
      self.openmp = True
    else:
      self.openmp = False
    for option in benchmark_options:
      if option.startswith('pinthreads'):
        if '=' in option:
          corelist = option.split('=')[1]
        else:
          corelist = ','.join(map(str, range(nthreads)))
        os.environ['PARMACS_PINTHREADS'] = corelist
    # do the tests in self.nthreads, and fail early if we're called with an unsupported (program, nthreads, inputsize) combination
    nthreads = self.get_nthreads()
    # Check other constraints
    if self.program == 'facesim':
      nthreads_supported = (1, 2, 3, 4, 6, 8, 16, 32, 64, 128)
      if nthreads not in nthreads_supported:
        raise ValueError("Benchmark %s only supports running with %s threads" % (self.program, nthreads_supported))
    elif self.program == 'fluidanimate':
      # nthreads must be power of two, one master thread will be added
      if nthreads != 1 << log2(nthreads):
        raise ValueError("Benchmark %s: number of threads must be power of two" % self.program)



  def get_nthreads(self):
    if self.nthreads_force:
      return self.nthreads

    if self.program == 'blackscholes':
      nthreads = self.nthreads - 1
    elif self.program == 'bodytrack':
      nthreads = self.nthreads - 2
    elif self.program == 'facesim':
      nthreads = self.nthreads
    elif self.program == 'ferret':
      nthreads = (self.nthreads - 2) / 4
    elif self.program == 'fluidanimate':
      # nthreads must be power of two, one master thread will be added
      nthreads = 1 << log2(self.nthreads - 1)
    elif self.program == 'swaptions':
      nthreads = self.nthreads - 1
    elif self.program == 'canneal':
      nthreads = self.nthreads - 1
    elif self.program == 'raytrace':
      nthreads = self.nthreads - 1
    elif self.program == 'dedup':
      nthreads = self.nthreads / 4
    elif self.program == 'streamcluster':
      nthreads = self.nthreads - 1
    elif self.program == 'vips':
      nthreads = self.nthreads - 2
    else:
      nthreads = self.nthreads

    if nthreads < 1:
      raise ValueError("Benchmark %s needs more cores" % self.program)

    return nthreads


  def run(self, graphitecmd, postcmd = ''):
    if postcmd != '':
      sys.stderr.write('PARSEC Error: postcmd not supported\n')
      return 1

    flags = []
    rundir = tempfile.mkdtemp()

    if self.program == 'facesim':
      # Facesim needs a {rundir}/Storytelling/output directory and tries to create it with a system() call
      # -- which doesn't work under Graphite.
      # Therefore: set up the complete rundir ourselves, including input files and Storytelling/output
      inputfile = '%s/parsec-2.1/pkgs/apps/facesim/inputs/input_%s.tar' % (HOME, self.inputsize)
      if not os.path.exists(inputfile):
        print 'Cannot find input file %(inputfile)s' % locals()
        sys.exit(-1)
      flags.append('-k')
      os.system('rm -r %(rundir)s/apps/facesim/run' % locals())
      os.system('mkdir -p %(rundir)s/apps/facesim/run/Storytelling/output' % locals())
      os.system('tar xvf %(inputfile)s -C %(rundir)s/apps/facesim/run' % locals())

    if self.openmp:
      os.putenv('OMP_NUM_THREADS', str(self.get_nthreads()))
    proc = subprocess.Popen([ '%s/parsec-2.1/bin/parsecmgmt' % HOME,
                         '-a', 'run', '-p', self.program, '-c', PLATFORM, '-i', self.inputsize, '-n', str(self.get_nthreads()),
                         '-s', graphitecmd, '-d', rundir
                     ] + flags)
    proc.communicate()

    os.system('rm -r %(rundir)s' % locals())
    return proc.returncode

  def rungraphiteoptions(self):
    return ''
