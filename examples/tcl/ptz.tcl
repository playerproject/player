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
# $Id$
#
# ptz.tcl
#
#  a simple demo to show how to send commands to and get feedback from
#  the Sony PTZ camera.  this program will pan the camera in a loop
#  from side to side
#

# to get the 'sleep' command
package require Tclx

set PLAYER_VERSION 1.0a

#
# get the robot client utilities
if {[file exists ../../fclient_libs/newtcl/playerclient.tcl]} {
  source ../../client_libs/newtcl/playerclient.tcl
} else { 
  source /usr/local/player-${PLAYER_VERSION}/lib/playerclient.tcl
}

set USAGE "USAGE: ptz.tcl \[-h <host>\] \[-p <port>\]"

set host "localhost"
set port $PLAYER_DEFAULT_PORT

set i 0
while {$i < $argc} {
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

player_connect -reqrep robot $host $port
if {[string compare [player_req_dev robot ptz a] a]} {
  error "couldn't get ptz access"
}

set dir 1
while {1} {
  player_read robot
  puts "pan:$robot(ptz,pan)  tilt:$robot(ptz,tilt)  zoom:$robot(ptz,zoom)"
  if {$robot(ptz,pan) > 80 || $robot(ptz,pan) < -80} {
    set dir -$dir
    player_set_camera robot [expr $robot(ptz,pan) + ($dir * 10)] 0 0
    sleep 1
    player_read robot
  }
  player_set_camera robot [expr $robot(ptz,pan) + ($dir * 5)] 0 0
}


