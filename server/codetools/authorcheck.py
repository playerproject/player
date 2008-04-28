#!/usr/bin/env python

import sys
import popen2

USAGE = 'USAGE: authorcheck.py <path> > outfile'


if len(sys.argv) < 2:
  print USAGE
  sys.exit(-1)

prefix = '/code/player/trunk/'

cmdline = 'svn log -v ' + sys.argv[1]

outf, inf = popen2.popen2(cmdline)

outp = outf.read()

outl = outp.split('\n')

db = dict()

awaiting_dashes = -1
awaiting_rev = 0
awaiting_changed = 1
reading_files = 2
state = awaiting_dashes
author = ''

for line in outl:
  lsplit = line.split()

  if state == awaiting_dashes:
    if line == '------------------------------------------------------------------------':
      state = awaiting_rev
  elif state == awaiting_rev:
    if len(line) == 0:
      continue
    if line[0] == 'r':
      author = lsplit[2]
      state = awaiting_changed
  elif state == awaiting_changed:
    state = reading_files
  else:
    if len(line) == 0:
      state = awaiting_dashes
    else:
      file = lsplit[1]
      if file[len(prefix):len(prefix)+len(sys.argv[1])] == sys.argv[1] and file.find('.') != -1 and (file[-1] == 'c' or file[-1] == 'h' or file[-3:] == 'cpp'):
        file = file[len(prefix):]
        if not db.has_key(author):
          db[author] = set()
        db[author].add(file)

for f in db:
  print '----------------\n'
  print f + ':'
  for a in db[f]:
    print a
  print ''
