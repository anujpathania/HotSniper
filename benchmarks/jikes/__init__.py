import jikes


def allbenchmarks():
  return jikes.benchmarks.keys()


def allinputs():
  return []


class Program:

  def __init__(self, program, nthreads, inputsize, benchmark_options = []):
    if program not in allbenchmarks():
      raise ValueError("Invalid benchmark %s" % program)
    self.program = program
    self.nthreads = nthreads
    self.inputsize = inputsize
    self.benchmark_options = benchmark_options

  def run(self, graphitecmd, postcmd = ''):
    if postcmd != '':
      sys.stderr.write('JIKES Error: postcmd not supported\n')
      return 1

    options = []
    advice = True
    gcthreads = self.nthreads
    nruns = 2
    for opt in self.benchmark_options:
      if opt == 'replay=false':
        advice = False
      elif opt.startswith('gcthreads='):
        gcthreads = int(opt.split('=')[1])
      elif opt.startswith('nruns='):
        nruns = int(opt.split('=')[1])
      else:
        options.append(opt)

    return jikes.run_jikes(graphitecmd, '', self.program, self.nthreads, gcthreads, options, nruns = nruns, advice = advice)

  def rungraphiteoptions(self):
    return ''
