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


# $Id$
#
# sonarobstacleavoid.tcl
#
#  a simple demo to do (dumb) sonarobstacleavoidance
#

#
# get the robot client utilities
lappend auto_path ../../client_libs
package require Tclplayer

set USAGE "USAGE: sonarobstacleavoid.tcl \[-h <host>\] \[-p <port>\]"

set host "localhost"
set port $PLAYER_DEFAULT_PORT
set key ""

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
  } elseif {![string compare [lindex $argv $i] "-k"]} {
    incr i
    if {$i == $argc} {
      puts $USAGE 
      exit 1
    }
    set key [lindex $argv $i]
  } else {
    puts $USAGE
    exit 1
  }
  incr i
}


player_connect robot $host $port
if {[string length $key]} {
  player_authenticate robot $key
}
if {[string compare [player_req_dev robot sonar r] r]} {
  error "couldn't get sonar read access"
}
if {[string compare [player_req_dev robot position w] w]} {
  error "couldn't get position write access"
}

set min_front_dist 500
set really_min_front_dist 300
set avoid 0

set newspeed 0 
set newturnrate 0
while {1} {
    # this blocks until new data comes; 10Hz by default 
    player_read robot

     #
     # sonar avoid.
     #   policy (pretty stupid):
     #     if(object really close in front)
     #       backup and turn away;
     #     else if(object close in front)
     #       stop and turn away;
    set avoid 0
    set newspeed 200

    if {!$avoid} {
      if {($robot(sonar,2) < $really_min_front_dist) || \
          ($robot(sonar,3) < $really_min_front_dist) || \
          ($robot(sonar,4) < $really_min_front_dist) || \
          ($robot(sonar,5) < $really_min_front_dist)} {
            set avoid 50
            set newspeed -100
       } elseif {($robot(sonar,2) < $min_front_dist) || \
                 ($robot(sonar,3) < $min_front_dist) || \
                 ($robot(sonar,4) < $min_front_dist) || \
                 ($robot(sonar,5) < $min_front_dist)} {
            set newspeed 0
            set avoid 50
       }
    }

    if {$avoid > 0} {
      if {($robot(sonar,0) + $robot(sonar,1)) < \
          ($robot(sonar,6) + $robot(sonar,7))} {
        set newturnrate 30
      } else {
        set newturnrate -30
      }
      incr avoid -1
    } else {
      set newturnrate 0
    }
    #parray robot

    # write commands to robot
    #puts "$newspeed $newturnrate"
    player_set_speed robot $newspeed $newturnrate
  }
}
