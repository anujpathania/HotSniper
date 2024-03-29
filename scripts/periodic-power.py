"""
stattrace.py

Write a trace of deltas for an arbitrary statistic.
First argument is the name of the statistic (<component-name>[.<subcomponent>].<stat-name>)
Second argument is either a filename, or none to write to standard output
Third argument is the interval size in nanoseconds (default is 10000)
"""

import sys, os, sim

class StatTrace:
  def setup(self, args):
    args = dict(enumerate((args or '').split(':')))
    stat = 'core.energy-dynamic'
    filename = 'Temp'

    interval_ns = long(args.get(0, 100000))

    self.clean_files()

    if '.' not in stat:
      print 'Stat name needs to be of the format <component>.<statname>, now %s' % stat
      return
    self.stat_name = stat
    stat_component, stat_name = stat.rsplit('.', 1)

    valid = False
    for core in range(sim.config.ncores):
      try:
        sim.stats.get(stat_component, core, stat_name)
      except ValueError:
        continue
      else:
        valid = True
        break
    if not valid:
      print 'Stat %s[*].%s not found' % (stat_component, stat_name)
      return

    if filename:
      self.fd = file(os.path.join(sim.config.output_dir, filename), 'w')
      self.isTerminal = False
    else:
      self.fd = sys.stdout
      self.isTerminal = True

    self.sd = sim.util.StatsDelta()
    self.stats = {
      'time': [ self.getStatsGetter('performance_model', core, 'elapsed_time') for core in range(sim.config.ncores) ],
      'ffwd_time': [ self.getStatsGetter('fastforward_performance_model', core, 'fastforwarded_time') for core in range(sim.config.ncores) ],
      'stat': [ self.getStatsGetter(stat_component, core, stat_name) for core in range(sim.config.ncores) ],
    }

    sim.util.Every(interval_ns * sim.util.Time.NS, self.periodic, statsdelta = self.sd, roi_only = True)
    
  def clean_files(self):
    # gkothar1
    for filename in (os.path.join(sim.config.output_dir, 'PeriodicCPIStack.log'),
                     os.path.join(sim.config.output_dir, 'PeriodicPower.log'),
                     os.path.join(sim.config.output_dir, 'PeriodicThermal.log'),
                     os.path.join(sim.config.output_dir, 'PeriodicFrequency.log'),
                     os.path.join(sim.config.output_dir, 'PeriodicVdd.log'),
                     os.path.join(sim.config.output_dir, 'PeriodicRvalue.log')):
      open(filename, 'w')  # empties the file

      # The following files need to be *removed* not just emptied.
      sum_file = os.path.join(sim.config.output_dir,
              sim.config.get('reliability/sum_file'))
      rvalues_file = os.path.join(sim.config.output_dir, "InstantaneousRvalue.log")
      if os.path.exists(sum_file):
          #print("DEBUG: removing {}".format(sum_file))
          os.remove(sum_file)
      if os.path.exists(rvalues_file):
          #print("DEBUG: removing {}".format(rvalues_file))
          os.remove(rvalues_file)


  def periodic(self, time, time_delta):
    if self.isTerminal:
      self.fd.write('[STAT:%s] ' % self.stat_name)
    self.fd.write('%u' % (time / 1e6)) # Time in ns
    for core in range(sim.config.ncores):
      timediff = (self.stats['time'][core].delta - self.stats['ffwd_time'][core].delta) / 1e6 # Time in ns
      statdiff = self.stats['stat'][core].delta
      value = statdiff / (timediff or 1) # Avoid division by zero
      self.fd.write(' %.3f' % value)
    self.fd.write('\n')

  def getStatsGetter(self, component, core, metric):
    # Some components don't exist (i.e. DRAM reads on cores that don't have a DRAM controller),
    # return a special object that always returns 0 in these cases
    try:
      return self.sd.getter(component, core, metric)
    except:
      class Zero():
        def __init__(self): self.delta = 0
        def update(self): pass
      return Zero()

sim.util.register(StatTrace())
