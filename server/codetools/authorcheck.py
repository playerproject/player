#!/usr/bin/env python

# *  Player - One Hell of a Robot Server
# *  Copyright (C) 2008
# *     Brian Gerkey
# *
# *
# *  This library is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU Lesser General Public
# *  License as published by the Free Software Foundation; either
# *  version 2.1 of the License, or (at your option) any later version.
# *
# *  This library is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# *  Lesser General Public License for more details.
# *
# *  You should have received a copy of the GNU Lesser General Public
# *  License along with this library; if not, write to the Free Software
# *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
