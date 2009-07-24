#!/usr/bin/ruby
#/*
# *  Player - One Hell of a Robot Server
# *  Copyright (C) 2009
# *     Jordi Polo
# *                      
# *
# *  This library is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU Lesser General Public
# *  License as published by the Free Software Foundation; either
# *  version 2.1 of the License, or (at your option) any later version.
# *
# *  This library is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# *  Lesser General Public License for more details.
# *
# *  You should have received a copy of the GNU Lesser General Public
# *  License along with this library; if not, write to the Free Software
# *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# */
def error
  puts "Player bindings not installed"
end

begin
  
  require 'playercpp'

  if Playercpp::constants.size > 0
    puts "Player bindings are installed"
    classes = Playercpp.constants.select {|c| Class === Playerc.const_get(c)}
#    puts classes
  else
    error
  end
rescue LoadError
  error
rescue
  error
end


