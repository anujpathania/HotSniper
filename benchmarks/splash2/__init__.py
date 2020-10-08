import sys, os, time, getopt, subprocess


abspath = lambda d: os.path.abspath(os.path.join(d))

HOME = abspath(os.path.dirname(__file__))

splashrun = {}
execfile(os.path.join(HOME, 'splash2', 'splashrun'), splashrun)


def allbenchmarks():
  benchmarks = subprocess.Popen([ '%(HOME)s/splash2/splashrun' % globals(), '-l' ], stdout = subprocess.PIPE).communicate()[0]
  return benchmarks.split()


def allinputs():
  return ('test', 'tiny', 'small', 'large')



class Program:

  def __init__(self, program, nthreads, inputsize, benchmark_options = []):
    if program not in allbenchmarks():
      raise ValueError("Invalid benchmark %s" % program)
    if inputsize not in allinputs():
      raise ValueError("Invalid input size %s" % inputsize)
    self.program = program
    self.nthreads = int(nthreads)
    self.inputsize = inputsize
    splashrun['benchmarks'][self.program].validate(self.inputsize, self.nthreads)


  def run(self, graphitecmd, postcmd = ''):
    if postcmd != '':
      sys.stderr.write('Splash2 run error: postcmd currently not supported\n')
      return 1
    proc = subprocess.Popen([ '%s/splash2/splashrun' % HOME,
                         '-p', self.program, '-i', self.inputsize, '-n', str(self.nthreads),
                         '-s', graphitecmd
                     ])
    proc.communicate()
    return proc.returncode


  def rungraphiteoptions(self):
    return ''
