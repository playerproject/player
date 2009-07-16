#!/usr/bin/env python

import math
from playerc import *

def limit(val, bottom, top):
    if val > top:
        return top
    elif val < bottom:
        return bottom
    return val

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
#print 'Laser pose: (%.3f,%.3f,%.3f)' % (l.pose[0],l.pose[1],l.pose[2])

while True:
    if c.read() == None:
        raise playerc_error_str()
    minR = l.min_right
    minL = l.min_left

    print "minR:", minR, "minL:", minL
    left = (1e5*minR)/500-100
    right = (1e5*minL)/500-100

    if left > 100:
        left = 100
    if right > 100:
        right = 100

    newspeed = (right+left)/1e3

    newturnrate = (right-left)
    newturnrate = limit(newturnrate, -40.0, 40.0)
    newturnrate = math.radians(newturnrate)

    print "speed: ", newspeed, "turn: ", newturnrate
    print help(p.set_cmd_vel)

    p.set_cmd_vel(newspeed, newspeed, newturnrate, 1)

# Clean up
p.unsubscribe()
l.unsubscribe()
c.disconnect()


 	  	 
