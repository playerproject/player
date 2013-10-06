#!/usr/bin/env python

#Copyright (c) 2006, Brian Gerkey
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without 
#modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice, 
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#    * Neither the name of the Player Project nor the names of its contributors 
#      may be used to endorse or promote products derived from this software 
#      without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import math
from playerc import *

# Create a client object
c = playerc_client(None, 'localhost', 6665)
# Connect it
if c.connect() != 0:
  raise playerc_error_str()

# Create a proxy for position2d:0
p = playerc_position2d(c,0)
if p.subscribe(PLAYERC_OPEN_MODE) != 0:
  raise playerc_error_str()

# Retrieve the geometry
if p.get_geom() != 0:
  raise playerc_error_str()
print 'Robot size: (%.3f,%.3f)' % (p.size[0], p.size[1])

# Create a proxy for laser:0
l = playerc_laser(c,0)
if l.subscribe(PLAYERC_OPEN_MODE) != 0:
  raise playerc_error_str()

# Retrieve the geometry
if l.get_geom() != 0:
  raise playerc_error_str()
print 'Laser pose: (%.3f,%.3f,%.3f)' % (l.pose[0],l.pose[1],l.pose[2])

# Start the robot turning CCW at 20 deg / sec
p.set_cmd_vel(0.0, 0.0, 20.0 * math.pi / 180.0, 1)

for i in range(0,30):
  if c.read() == None:
    raise playerc_error_str()

  print 'Robot pose: (%.3f,%.3f,%.3f)' % (p.px,p.py,p.pa)
  laserscanstr = 'Partial laser scan: '
  for j in range(0,5):
    if j >= l.scan_count:
      break
    laserscanstr += '%.3f ' % l.ranges[j]
  print laserscanstr

# Now turn the other way
p.set_cmd_vel(0.0, 0.0, -20.0 * math.pi / 180.0, 1)

for i in range(0,30):
  if c.read() == None:
    raise playerc_error_str()

  print 'Robot pose: (%.3f,%.3f,%.3f)' % (p.px,p.py,p.pa)
  laserscanstr = 'Partial laser scan: '
  for j in range(0,5):
    if j >= l.scan_count:
      break
    laserscanstr += '%.3f ' % l.ranges[j]
  print laserscanstr

# Now stop
p.set_cmd_vel(0.0, 0.0, 0.0, 1)

# Clean up
p.unsubscribe()
l.unsubscribe()
c.disconnect()

