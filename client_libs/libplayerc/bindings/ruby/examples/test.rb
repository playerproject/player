#!/usr/bin/ruby

require 'playerc'
require 'laser'
#require 'wifi'

class PlayercExamples
  def initialize 
    @connection = Playerc::Playerc_client.new(nil, 'localhost', 6665)
    if @connection.connect != 0
      raise Playerc::playerc_error_str()
    end
    puts 'connected to player'
  end

  def finish
    @connection.disconnect()
    puts 'disconnected from player'
  end


end


examples = PlayercExamples.new
tests = []

ARGV.each do|a|
  if (a == "all")
    tests = examples.public_methods false
  else
    if examples.respond_to?(a.to_sym)
      tests.push(a)
    else
      puts "requested test #{a} doesn't exist"
    end
  end
  tests.reject! {|e| e == 'finish'} 
end

tests.each do |test|
  examples.send(test.to_sym)
end

examples.finish


