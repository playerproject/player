#!/usr/bin/ruby

require 'playerc'
require 'laser'
require 'fiducial'
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

if ARGV.count == 0
  puts "     usage: ruby test.rb NAME_OF_TEST Arguments"
  puts "        ex: ruby test.rb laser"
  puts "            ruby test.rb all    for running all the tests"

end

ARGV.each do|a|
#a = ARGV[0]
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


