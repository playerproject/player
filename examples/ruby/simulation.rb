#testing a fiducial interface

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
