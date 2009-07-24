#!/usr/bin/ruby

require 'playercpp'

# Create a client object and connect it
robot = Playercpp::PlayerClient('localhost')

# Create a proxy for position2d:0
pp = Playercpp::Position2dProxy(robot, 0)

# Retrieve the geometry
pp.RequestGeom()
size = pp.GetSize()
puts 'Robot size: (%.3f,%.3f,%.3f)' % [size.sw, size.sl, size.sh]

# Create a proxy for laser:0
lp = Playercpp::LaserProxy(robot, 0)

# Retrieve the geometry
pose = lp.GetSize()
puts 'Laser pose: (%.3f,%.3f,%.3f)' % [pose.sw, pose.sl, pose.sh]

# Start the robot turning CCW at 20 deg / sec
pp.SetSpeed(0.0, 20.0 * Math::PI / 180.0)

for i in 0..30 do
  robot.Read()
  px = pp.GetXPos()
  py = pp.GetYPos()
  pa = pp.GetYaw()
  puts 'Robot pose: (%.3f,%.3f,%.3f)' % [px, py, pa]
  laserscanstr = 'Partial laser scan: '
  for j in 0..5 do
    break if j >= lp.GetCount()
    laserscanstr += '%.3f ' % lp.GetRange(j)
  end
  puts laserscanstr
end

# Now turn the other way
pp.SetSpeed(0.0, - 20.0 * Math::PI / 180.0)

for i in 0..30 do
  robot.Read()
  px = pp.GetXPos()
  py = pp.GetYPos()
  pa = pp.GetYaw()
  puts 'Robot pose: (%.3f,%.3f,%.3f)' % [px, py, pa]
  laserscanstr = 'Partial laser scan: '
  for j in 0..5 do
    break if j >= lp.GetCount()
    laserscanstr += '%.3f ' % lp.GetRange(j)
  end
  puts laserscanstr
end

# Now stop
pp.SetSpeed(0.0, 0.0)

# TODO: we can/should proxies and robot explicity?
del pp
del lp
del robot
                                         
