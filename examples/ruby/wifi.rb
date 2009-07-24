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

  def wifi

    wifi = Playerc::Playerc_wifi.new(@connection, 0)

    #for i in range(device.link_count + 10):
    #    link = playerc_wifi_get_link(device, i)
    #    #print '[%s] [%d]' % (link.mac, link.level)

    link = wifi.get_link(0)
    puts 'wifi mac: [%s], ip [%s], essid [%s], wifi level: [%d]' % [link.mac, link.ip, link.essid, link.level] unless link.nil?

    if wifi.subscribe( Playerc::PLAYERC_OPEN_MODE) != 0 
      raise Playerc::playerc_error_str
    end


    for i in 0..5 do
      if @connection.read.nil?
        raise Playerc::playerc_error_str()
      end
      puts wifi.scan
      for link in device.links do
        puts "wifi: [%d] [%s] [%s] [%s] [%4d] [%4d]" % \
          [i, link.mac, link.essid, link.ip, link.level, link.noise]
      end
    end

  end
end
