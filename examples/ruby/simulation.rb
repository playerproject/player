#testing a fiducial interface
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
require 'playerc'

class PlayercExamples 

  def simulation
    if @connection.nil? 
      raise 'our connection is not valid!'
    end
    simulation = Playerc::Playerc_simulation.new(@connection, 0)
    if simulation.subscribe(Playerc::PLAYER_OPEN_MODE) != 0
      raise  Playerc::playerc_error_str()
    end

#    if @connection.read.nil?
#      raise Playerc::playerc_error_str()
#    end
    pose = simulation.get_pose2d("pioneer2dx_model1") 
    puts "robot global coordinates, x #{pose[1]}, y #{pose[2]},  yaw #{pose[3]}"

    methods = simulation.public_methods(false).sort.map {|e| e.to_s + ", "}.to_s
    
    p "simulation interface is cool, methods available: ",  methods

    simulation.unsubscribe()

  end
end
