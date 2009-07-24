#! /usr/bin/env python

import math
from playercpp import *

# Create a client object and connect it
robot = PlayerClient('localhost')

# Create a proxy for position2d:0
pp = Position2dProxy(robot, 0)

# Retrieve the geometry
pp.RequestGeom()
size = pp.GetSize()
print 'Robot size: (%.3f,%.3f,%.3f)' % (size.sw, size.sl, size.sh)

# Create a proxy for laser:0
lp = LaserProxy(robot, 0)

# Retrieve the geometry
pose = lp.GetSize()
print 'Laser pose: (%.3f,%.3f,%.3f)' % (pose.sw, pose.sl, pose.sh)

# Start the robot turning CCW at 20 deg / sec
pp.SetSpeed(0.0, 20.0 * math.pi / 180.0)

for i in range(0,30):
  robot.Read()
  px = pp.GetXPos()
  py = pp.GetYPos()
  pa = pp.GetYaw()
  print 'Robot pose: (%.3f,%.3f,%.3f)' % (px, py, pa)
  laserscanstr = 'Partial laser scan: '
  for j in range(0,5):
    if j >= lp.GetCount():
      break
    laserscanstr += '%.3f ' % lp.GetRange(i)
  print laserscanstr

# Now turn the other way
pp.SetSpeed(0.0, - 20.0 * math.pi / 180.0)

for i in range(0,30):
  robot.Read()
  px = pp.GetXPos()
  py = pp.GetYPos()
  pa = pp.GetYaw()
  print 'Robot pose: (%.3f,%.3f,%.3f)' % (px, py, pa)
  laserscanstr = 'Partial laser scan: '
  for j in range(0,5):
    if j >= lp.GetCount():
      break
    laserscanstr += '%.3f ' % lp.GetRange(i)
  print laserscanstr

# Now stop
pp.SetSpeed(0.0, 0.0)

# TODO: we can/should proxies and robot explicity?
del pp
del lp
del robot
