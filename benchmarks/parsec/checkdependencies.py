#!/usr/bin/env python

import sys, os

HOME = os.path.realpath(os.path.dirname(__file__))
options = eval(open(os.path.join(HOME, '../tools/hooks', 'buildconf.py')).read())
static = options.get('USE_STATIC', False)


# list of (packagename, filename)


if os.path.exists('/etc/redhat-release'):
  ostype = 'redhat'
elif os.path.exists('/etc/debian_version'):
  ostype = 'debian'
else:
  ostype = ''


DEPENDENCIES = [
  ('/usr/bin/xsltproc',       {'debian':'xsltproc',     'redhat':'libxslt'}),
  ('/usr/bin/pkg-config',     {'debian':'pkg-config',   'redhat':'pkgconfig'}),
  ('/usr/bin/gettextize',     {'debian':'gettext',      'redhat':'gettext'}),
  ('/usr/lib/libX11.so',      {'debian':'libx11-dev',   'redhat':'libX11-devel'}),
  ('/usr/lib/libXext.so',     {'debian':'libxext-dev',  'redhat':'libXext-devel'}),
  ('/usr/lib/libXt.so',       {'debian':'libxt-dev',    'redhat':'libXt-devel'}),
  ('/usr/lib/libXmu.so',      {'debian':'libxmu-dev',   'redhat':'libXmu-devel'}),
  ('/usr/lib/libXi.so',       {'debian':'libxi-dev',    'redhat':'libXi-devel'}),
  ('/usr/bin/patch',          {'debian':'patch',        'redhat':'patch'}),
]
if static:
  DEPENDENCIES += [
    ('/usr/lib/libjpeg.a',      {'debian':'libjpeg-dev',  'redhat':'libjpeg-static'}),
    ('/usr/lib/libglut.a',      {'debian':'libglut3-dev', 'redhat':'freeglut-devel'}),
  ]


missing = []

for filename, package in DEPENDENCIES:
  if not os.path.exists(filename) and \
     not os.path.exists(filename.replace('/usr/lib', '/usr/lib64')) and \
     not os.path.exists(filename.replace('/usr/lib', '/usr/lib/x86_64-linux-gnu')) and \
     not os.path.exists(filename.replace('/usr/lib', '/usr/lib/i386-linux-gnu')) \
  :
    if ostype:
      missing.append(package[ostype])
    else:
      print '*** Cannot find the file %s' % filename

if missing:
  print >> sys.stderr, '*** Please install the following package%s: %s' % \
                       (len(missing)>1 and 's' or '', ' '.join(missing))
  sys.exit(1)
else:
  sys.exit(0)
