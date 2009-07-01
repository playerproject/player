#testing a fiducial interface

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
      raise "fiducial not reading anything"
    end
 
    for i in 0..fiducial.fiducials_count do
      f = fiducial.fiducials[i]
      puts "id, x, y, range, bearing, orientation: ", f.id, f.pos[0], f.pos[1], f.range, f.bearing * 180 / PI, f.orient
    end 
 
    fiducial.unsubscribe()

  end
end
