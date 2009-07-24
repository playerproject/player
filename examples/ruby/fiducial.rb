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

  def fiducial
    if @connection.nil? 
      raise 'our connection is not valid!'
    end
    fiducial = Playerc::Playerc_fiducial.new(@connection, 0)
    if fiducial.subscribe(Playerc::PLAYER_OPEN_MODE) != 0
      raise  Playerc::playerc_error_str()
    end

    if @connection.read.nil?
      raise Playerc::playerc_error_str()
    end
 
    puts "fiducial device with #{fiducial.fiducials_count} readings"

    if fiducial.fiducials_count == 0 
      puts "no readings available in this interface"
    else
#TODO: more than one object found?
#      for i in 0..fiducial.fiducials_count do
         f = fiducial.fiducials
         puts "object found" 
         puts "object id: #{f.id}, x: #{f.pose.px}, y: #{f.pose.py}, angle: #{f.pose.pyaw}"
#        f = fiducial.fiducials[i]
#        puts "id, x, y, range, bearing, orientation: ", f.id, f.pos[0], f.pos[1], f.range, f.bearing * 180 / PI, f.orient
#      end 
    end
    fiducial.unsubscribe()

  end
end
