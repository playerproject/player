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

#This file is equivalent to playercpy_example.py of the Python bindings
#If the behaviour is not the same, something went wrong!

require 'playerc'

class PlayercExamples 

  def laser
    if @connection.nil? 
      raise 'our connection is not valid!'
    end
    position = Playerc::Playerc_position2d.new(@connection, 0)
    if position.subscribe(Playerc::PLAYER_OPEN_MODE) != 0
      raise  Playerc::playerc_error_str()
    end

    # Retrieve the robot geometry
    if position.get_geom != 0
      raise Playerc::playerc_error_str()
    end

    puts "Robot size:  #{position.size[0]}  #{position.size[1]} "

     # Create a proxy for laser:0
    laser = Playerc::Playerc_laser.new(@connection, 0)
    if laser.subscribe(Playerc::PLAYER_OPEN_MODE) != 0
      raise  Playerc::playerc_error_str()
    end

    # Retrieve the laser geometry
    if laser.get_geom != 0
      raise Playerc::playerc_error_str
    end
    puts "Laser pose: (%.3f, %.3f, %.3f)" % laser.pose 
    # Start the robot turning CCW at 20 deg / sec
    position.set_cmd_vel(0.0, 0.0, 20.0 * Math::PI / 180.0, 1)

    for i in 0..30 do
      if @connection.read.nil?
        raise Playerc::playerc_error_str()
      end

      puts "Robot pose: (%.3f,%.3f,%.3f)" % [position.px, position.py, position.pa]

      laserscanstr = 'Partial laser scan: '
      for j in 0..5 do
        break if j >= laser.scan_count
        laserscanstr += '%.3f ' % laser.ranges[j]
      end
     puts laserscanstr
     #times and scan counts
#     puts "laser datatime, scan count: (%14.3f) (%d)" % [laser.info.datatime, laser.scan_count]
#     puts " "
    end

    # Turn the other way
    position.set_cmd_vel(0.0, 0.0, -20.0 * Math::PI / 180.0, 1)

    for i in 0..30 do
      if @connection.read.nil?
        raise Playerc::playerc_error_str()
      end
      puts "Robot pose: (%.3f,%.3f,%.3f)" % [position.px,position.py,position.pa]
      laserscanstr = 'Partial laser scan: '
      for j in 0..5 do
        break if j >= laser.scan_count
        laserscanstr += '%.3f ' % laser.ranges[j]
      end
      puts laserscanstr
    end

    # Now stop
    position.set_cmd_vel(0.0, 0.0, 0.0, 1)

    # Clean up
    laser.unsubscribe()
    position.unsubscribe()
    

  end
end
