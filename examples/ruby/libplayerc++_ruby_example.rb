#!/usr/bin/ruby
#Copyright (c) 2009, Jordi Polo
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
                                         
