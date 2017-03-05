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

require 'playerc'
require 'laser'
require 'fiducial'
require 'simulation'
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


