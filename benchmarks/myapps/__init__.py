import os
import subprocess
import sys

abspath = lambda d: os.path.abspath(os.path.join(d))


HOME = abspath(os.path.dirname(__file__))



class Program:
  def __init__(self, program, nthreads, inputsize, benchmark_options=[]):
    self.program = program
    self.nthreads =  nthreads 
    self.inputsize = inputsize

  def run(self, graphitecmd, postcmd = ''):
    if postcmd != '':
      sys.stderr.write('Error: postcmd not supported\n')
      return 1
    if self.program == 'pi':
      os.putenv('OMP_NUM_THREADS', str(self.nthreads))
    apppath = (graphitecmd.split() + [('{}/' + self.program + '/' + self.program + ' ' + self.inputsize).format(HOME)])
    proc = subprocess.Popen(apppath)
    #subprocess.Popen(graphitecmd.split() + [('{}/' + self.program + '/' + self.program).format(HOME)])
    proc.communicate()

    return proc.returncode

  def rungraphiteoptions(self):
    return ''