import sys, os, subprocess, tempfile


abspath = lambda d: os.path.abspath(os.path.join(d))

HOME = abspath(os.path.dirname(__file__))
RVM_HOME = os.getenv('RVM_HOME', '%s/jikesrvm' % HOME)


benchmarks = {
  'pjbb2005':  { 'heapSize': '400M' },
  'pmd':       { 'heapSize': '98M' },
  'luindex':   { 'heapSize': '44M' },
  'jython':    { 'heapSize': '108M' },
  'sunflow':   { 'heapSize': '108M' },
  'xalan':     { 'heapSize': '108M' },
  'lusearch':  { 'heapSize': '68M' },
  'lusearch.fix':  { 'heapSize': '68M' },
  'avrora':    { 'heapSize': '100M' },
  'eclipse':   { 'heapSize': '160M' },
  'fop':       { 'heapSize': '80M' },
  'antlr':     { 'heapSize': '48M' },
  'bloat':     { 'heapSize': '66M' },
  'hsqldb':    { 'heapSize': '254M' },
}

APPS_912_BACH = ('pmd', 'luindex', 'jython', 'sunflow', 'xalan', 'lusearch', 'avrora')
APPS_200610_MR2 = ('fop', 'eclipse', 'antlr', 'bloat', 'hsqldb')


def get_rvm_cmdline(program, nthreads, gcthreads, benchmark_options = [], nruns = 2, advice = True):
  advicefile = program
  if program == 'pjbb2005':
    params = [
      '-cp', '%s/probes/probes.jar:%s/data/pjbb2005/jbb.jar:%s/data/pjbb2005/check.jar' % (HOME, HOME, HOME),
      ] + benchmark_options + [
      'spec.jbb.JBBmain', '-propfile', '%s/data/pjbb2005/SPECjbb-%dx%s.props' % (HOME, 8, 10000),
      '-c', 'PJBB2005Callback', '-n', str(nruns)
    ]
  elif program == 'lusearch.fix':
    params = [
      '-cp', '%s/probes/probes.jar:%s/data/dacapo-lucene-2-4-1-bugfree.jar' % (HOME, HOME),
      ] + benchmark_options + [
      'Harness', '-c', 'DacapoBachCallback',
      '-n%d' % nruns,
      'lusearch'
    ]
    advicefile = 'lusearchfix'
  elif program in APPS_912_BACH:
    params = [
      '-cp', '%s/probes/probes.jar:%s/data/dacapo-9.12-bach.jar' % (HOME, HOME),
      ] + benchmark_options + [
      'Harness', '-c', 'DacapoBachCallback',
      '-n%d' % nruns,
      program
    ]
  elif program in APPS_200610_MR2:
    params = [
      '-cp', '%s/probes/probes.jar:%s/data/dacapo-2006-10-MR2.jar' % (HOME, HOME),
      ] + benchmark_options + [
      'Harness', '-c', 'Dacapo2006Callback',
      '-n', str(nruns),
      program
    ]
  else:
    raise ValueError("Invalid benchmark %s" % program)

  rvm_args = [
    '%s/rvm' % RVM_HOME,
    '-wrap', 'echo',
    '-Xms%s' % benchmarks[program]['heapSize'],
    '-X:availableProcessors=%u' % nthreads,
    '-X:gc:threads=%u' % gcthreads,
    '-X:gc:variableSizeHeap=false',
    #'-X:vm:separateThread=Collector,Thread-,Query,node-,PmdThread,Lucene',
  ]
  if advice is True:
    # Replay compilation using stored advice files
    rvm_args += [
      '-Dprobes=Replay,MMTk',
      '-X:aos:initial_compiler=base',
      '-X:aos:enable_bulk_compile=true',
      '-X:aos:enable_recompilation=false',
      '-X:aos:cafi=%s/advice/%s.ca' % (HOME, advicefile),
      '-X:aos:dcfi=%s/advice/%s.dc' % (HOME, advicefile),
      '-X:vm:edgeCounterFile=%s/advice/%s.ec' % (HOME, advicefile),
    ]
  elif advice is False:
    # No replay compilation
    rvm_args += [
      '-Dprobes=MMTk',
      '-X:aos:enable_recompilation=true',
    ]
  else:
    # Generate new advice files, `advice' is output directory
    rvm_args += [
      '-Dprobes=MMTk',
      '-X:aos:enable_advice_generation=true',
      '-X:aos:cafo=%s/%s.ca' % (advice, advicefile),
      '-X:aos:dcfo=%s/%s.dc' % (advice, advicefile),
      '-X:base:profile_edge_counters=true',
      '-X:base:profile_edge_counter_file=%s/%s.ec' % (advice, advicefile),
      '-X:aos:final_report_level=2'
    ]

  proc = subprocess.Popen(rvm_args + [
                       ] + params, stdout = subprocess.PIPE)
  jikescmd = proc.communicate()[0].strip()

  return jikescmd


def run_jikes(wrapcmd, postcmd, *args, **kwds):
  origcwd = os.getcwd()
  # Make the new directories, and cd there
  rundir = tempfile.mkdtemp()
  os.chdir(rundir)

  jikescmd = get_rvm_cmdline(*args, **kwds)
  cmd = 'LD_LIBRARY_PATH=%s/jikesrvm:$LD_LIBRARY_PATH ' % HOME + wrapcmd + ' ' + jikescmd + ' ' + postcmd

  proc = subprocess.Popen(['bash', '-c', cmd])
  proc.communicate()

  os.chdir(origcwd)
  os.system('rm -rf "%s"' % rundir)

  return proc.returncode
