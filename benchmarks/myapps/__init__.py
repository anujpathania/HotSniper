import os
import subprocess
import sys

abspath = lambda d: os.path.abspath(os.path.join(d))


HOME = abspath(os.path.dirname(__file__))



class Program:
  def __init__(self, program, nthreads, inputsize, benchmark_options=[]):
    self.program = program
    #self.args = ' ' + nthreads + ' '


  def run(self, graphitecmd, postcmd = ''):
    if postcmd != '':
      sys.stderr.write('Error: postcmd not supported\n')
      return 1
    apppath = (graphitecmd.split() + [('{}/' + self.program + '/' + self.program ).format(HOME)])
    proc = subprocess.Popen(apppath)
    #subprocess.Popen(graphitecmd.split() + [('{}/' + self.program + '/' + self.program).format(HOME)])
    proc.communicate()

    return proc.returncode

  def rungraphiteoptions(self):
    return ''