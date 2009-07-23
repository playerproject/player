
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
