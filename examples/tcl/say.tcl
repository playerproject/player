#!/usr/bin/tclsh

#  Player - One Hell of a Robot Server
# Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
#                     gerkey@usc.edu    kaspers@robotics.usc.edu
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#
#
# say.tcl
#
#  a simple demo to show interface to the Festival speech device
#

# get the 'sleep' command
package require Tclx

#
# get the robot client utilities
if {[file exists ../../client_libs/tcl/playerclient.tcl]} {
  source ../../client_libs/tcl/playerclient.tcl
} else { 
  source /usr/local/player/lib/playerclient.tcl
}

set USAGE "USAGE: say.tcl \[-h <host>\] \[-p <port>\] <string>"

set host "localhost"
if {$argc < 1} {
  puts $USAGE
  exit 1
}
set saystring [lindex $argv end]

set i 0
while {$i < [expr $argc - 1]} {
  if {![string compare [lindex $argv $i] "-h"]} {
    incr i
    if {$i == $argc} {
      puts $USAGE 
      exit 1
    }
    set host [lindex $argv $i]
  } elseif {![string compare [lindex $argv $i] "-p"]} {
    incr i
    if {$i == $argc} {
      puts $USAGE 
      exit 1
    }
    set port [lindex $argv $i]
  } else {
    puts $USAGE
    exit 1
  }
  incr i
}

connectToRobot $host
requestDeviceAccess $PLAYER_SPEECH_CODE 0 $PLAYER_WRITE_MODE
writeCommand $PLAYER_SPEECH_CODE 0 $saystring
# why does this sleep need to be here?
sleep 1
disconnectFromRobot

