#!/usr/bin/env python

import os, sys, errno, getopt


def local_benchmarks_root():
  return os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))


def benchmarks_root():
  # Return an existing *_ROOT if it is valid
  for rootname in ('BENCHMARKS_ROOT',):
    root = os.getenv(rootname)
    if root:
      if not os.path.isfile(os.path.join(root,'run-sniper')):
        raise EnvironmentError((errno.EINVAL, 'Invalid %s directory [%s]' % (rootname, root)))
      else:
	if os.path.realpath(root) != local_benchmarks_root():
	  print >> sys.stderr, 'Warning: %s is different from current script directory [%s]!=[%s]' % (rootname, os.path.realpath(root), local_benchmarks_root())
	return root

  # Use the root corresponding to this file when nothing has been set
  return local_benchmarks_root()


def sniper_root():
  # Return an existing *_ROOT if it is valid
  for rootname in ('SNIPER_ROOT','GRAPHITE_ROOT'):
    root = os.getenv(rootname)
    if root:
      if not os.path.isfile(os.path.join(root,'run-sniper')):
        raise EnvironmentError((errno.EINVAL, 'Invalid %s directory [%s]' % (rootname, root)))
      else:
	return root

  # Try to determine what the SNIPER_ROOT should be if it is not set
  bench_root = benchmarks_root()
  snipertry = []
  snipertry.append(os.path.realpath(os.path.join(bench_root,'..','sniper','run-sniper')))
  snipertry.append(os.path.realpath(os.path.join(bench_root,'sniper','run-sniper')))
  snipertry.append(os.path.realpath(os.path.join(bench_root,'..','run-sniper')))

  for bt in snipertry:
    if os.path.isfile(bt):
      return os.path.dirname(bt)

  raise EnvironmentError((errno.EINVAL, 'Unable to determine the SNIPER_ROOT directory'))


def sim_root():
  return sniper_root()


if __name__ == "__main__":

    def usage():
      print 'Determine variable values, using the environment or directory names as appropriate'
      print ' Usage:'
      print '  %s [--benchmarks | --sniper | --sim ]'
      print ' Returns the benchmarks, or Sniper root path. Otherwise, returns a json-like dictionary'
      print ' with the detailed information.'

    try:
      opts, args = getopt.getopt(sys.argv[1:], '', ['benchmarks', 'sniper', 'sim'])
    except getopt.GetoptError, e:
      # print help information and exit:
      print e
      usage()
      sys.exit(1)

    found_opt = False

    for o, a in opts:
      if o == '--benchmarks':
        found_opt = True
        sys.stdout.write('%s' % benchmarks_root())
      elif o == '--sniper' or o == '--sim':
        found_opt = True
        try:
          sys.stdout.write('%s' % sniper_root())
        except EnvironmentError, e:
          sys.stderr.write('EnvironmentError: %s\n' % e)
          sys.exit(1)

    if not found_opt:
      roots = {'SNIPER_ROOT': '', 'GRAPHITE_ROOT': '', 'BENCHMARKS_ROOT': ''}
      roots['BENCHMARKS_ROOT'] = benchmarks_root()
      try:
        roots['SNIPER_ROOT'] = roots['GRAPHITE_ROOT'] = sniper_root()
      except EnvironmentError, e:
        pass
      print roots

    sys.exit(0)
