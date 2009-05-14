#!/usr/bin/ruby

def error
  puts "Player bindings not installed"
end

begin
  
  require 'playerc'

  if Playerc::constants.size > 0
    puts "Player bindings are installed"
    classes = Playerc.constants.select {|c| Class === Playerc.const_get(c)}
#    puts classes
  else
    error
  end
rescue LoadError
  error
rescue
  error
end


