import sys, os, time, getopt, subprocess

HOME = os.path.abspath(os.path.join(os.path.dirname(__file__)))


def allbenchmarks():
    return [ 'pi' ]

def allinputs():
  return ('test', 'small', 'large')


class Program:

  def benchmark(self, program, inputsize, nthreads):
    if program == 'pi':
      if inputsize == 'test':
        num_steps = '1000'
      elif inputsize == 'small':
        num_steps = '100000'
      elif inputsize == 'large':
        num_steps = '1000000'
      else:
        raise ValueError("Invalid input size %s" % inputsize)
      return '%s/pi/pi %s' % (HOME, num_steps)

    else:
      raise ValueError("Unknown program %s" % program)


  def __init__(self, program, nthreads, inputsize, benchmark_options = []):
    if program not in allbenchmarks():
      raise ValueError("Invalid benchmark %s" % program)
    if inputsize not in allinputs():
      raise ValueError("Invalid input size %s" % inputsize)
    self.program = program
    self.nthreads = nthreads
    self.inputsize = inputsize
    self.cmd = self.benchmark(self.program, self.inputsize, self.nthreads)


  def ncores(self):
    return self.nthreads


  def run(self, submit):
    print '[LOCAL]', '[========== Running benchmark', self.program, '==========]'
    cmd = submit + ' ' + self.cmd
    print '[LOCAL]', 'Running \'' + cmd + '\':'
    print '[LOCAL]', '[---------- Beginning of output ----------]'
    sys.stdout.flush()
    sys.stderr.flush()
    os.putenv('OMP_NUM_THREADS', str(self.ncores()))
    os.system(cmd)
    print '[LOCAL]', '[----------    End of output    ----------]'
    print '[LOCAL]', 'Done.'

  def rungraphiteoptions(self):
    return ''
