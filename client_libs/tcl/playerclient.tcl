#
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

# client utilities

# globals
set port 6665
if {![string length [info var sock]]} {
  set sock -1
}
set frontbumpers 0
set rearbumpers 0
set battery 0
array set sonar {}
array set laser {}
array set vision {}
set pan 0
set tilt 0
set zoom 0
set time 0
set xpos 0 
set ypos 0
set heading 0
set speed 0 
set turnrate 0
set compass 0
set stall 0
set host ""

set ACTS_NUM_CHANNELS 32
set ACTS_HEADER_SIZE [expr 2*$ACTS_NUM_CHANNELS]
set ACTS_BLOB_SIZE 10

proc connectToRobot {robot} {
  global sock port host
  puts "connectToRobot:robot:$robot"

  set sock [socket $robot $port]
  fconfigure $sock -blocking 1 -translation binary
  # make it request/reply
  requestFromRobot "xy[binary format S 2]r[binary format c 1]"
  set host $robot
}
proc disconnectFromRobot {} {
  global sock
  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }
  close $sock
  set sock -1
}
proc requestFromRobot {req} {
  global sock host
  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }
  set isdevicerequest 0

  #puts -nonewline "Requesting \"$req\" (length: [string length $req])"
  if {[string index $req 0] == "x"} {
    puts -nonewline $sock $req
  } else {
    set isdevicerequest 1
    puts -nonewline $sock "dr[binary format S [string length $req]]$req"

    if {[string first vr $req] != -1 || 
        [string first va $req] != -1 ||
        [string first vw $req] != -1} {
        puts "Since you want vision, i'll xhost the robot for you."
        catch {exec xhost $host}
    }
  }
  flush $sock

  if {$isdevicerequest} {
    while {1} {
      set reply [read $sock 3]
      if {[binary scan [string range $reply 1 2] S size] != 1} {
        error "failed to get size specifier. should probably disconnect"
      }
      #puts "size:$size"
      if {![string compare [string index $reply 0] r]} {
        break;
      }
      # eat other data
      read $sock $size
    }
    set reply [read $sock $size]
    if {[string length $reply] != $size} {
      error "expected $size bytes, but only got [string length $reply]"
    }
    if {![string compare $reply $req]} {
      #puts "- ok"
    } else {
      #puts "- but got \"$reply\" (length: [string length $reply])"
      error "bad reply \"$reply\""
    }
  }
}

proc readData {str} {
  global sock
  global battery frontbumpers rearbumpers
  global sonar
  global time xpos ypos heading speed turnrate compass stall
  global laser vision
  global pan tilt zoom
  global ACTS_BLOB_SIZE ACTS_NUM_CHANNELS ACTS_HEADER_SIZE

  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }

  set i 0
  # request a packet
  requestFromRobot "xy[binary format S 1]d"
  while {$i < [string length $str]} {

    set prefix [read $sock 3]
    #puts "prefix:$prefix"
    if {[string compare [string index $prefix 0] [string index $str $i]]} {
      puts "was expecting \"[string index $str $i]\", but got \"[string index $prefix 0]\""
    }
    if {[binary scan [string range $prefix 1 2] S size] != 1} {
      error "failed to get size specifier. should probably disconnect"
    }
  
    set data [read $sock $size]
    if {[string length $data] != $size} {
      error "expected $size bytes, but only got [string length $data]"
    }
  
    if {![string compare [string index $prefix 0] v]} {
      # vision data packet
      set bufptr $ACTS_HEADER_SIZE
      set l 0
      while {$l < $ACTS_NUM_CHANNELS} {
        if {[binary scan $data "x[expr 2*$l+1]c" numblobs] != 1} {
          error "failed to get number of blobs for channel $l"
        }
        set vision($l,numblobs) [expr $numblobs - 1]
        set j 0
        while {$j < $vision($l,numblobs)} {
          if {[binary scan $data "x[expr $bufptr]cccccccccc" \
                    areabits(0) areabits(1) areabits(2) areabits(3)\
                    x y left right top bottom] != 10} {
            error "failed to get blob info for ${j}th blob on ${l}th channel"
          }
          # make everything unsigned
          set x [expr ( $x + 0x100 ) % 0x100]
          set y [expr ( $y + 0x100 ) % 0x100]
          set left [expr ( $left + 0x100 ) % 0x100]
          set right [expr ( $right + 0x100 ) % 0x100]
          set top [expr ( $top + 0x100 ) % 0x100]
          set bottom [expr ( $bottom + 0x100 ) % 0x100]

          # first compute the area
          set area 0
          set k 0
          while {$k < 4} {
            set area [expr $area << 6]
            set area [expr $area | ($areabits($k) - 1)]
            incr k
          }
          set vision($l,$j,area) $area
          set vision($l,$j,x) [expr $x - 1]
          set vision($l,$j,y) [expr $y - 1]
          set vision($l,$j,left) [expr $left - 1]
          set vision($l,$j,right) [expr $right - 1]
          set vision($l,$j,top) [expr $top - 1]
          set vision($l,$j,bottom) [expr $bottom - 1]

          incr bufptr $ACTS_BLOB_SIZE
          incr j
        }
        incr l
      }
    } elseif {![string compare [string index $prefix 0] z]} {
      # ptz data packet
      if {[binary scan $data SSS pan tilt zoom] != 3} {
        error "failed to get pan/tilt/zoom"
      }
    } elseif {![string compare [string index $prefix 0] m]} {
      # misc data packet
      if {[binary scan $data ccc frontbumpers rearbumpers battery] != 3} {
        error "failed to get bumpers and battery"
      }
      # make them unsigned
      set frontbumpers [expr ( $frontbumpers + 0x100 ) % 0x100]
      set rearbumpers [expr ( $rearbumpers + 0x100 ) % 0x100]
      set battery [expr (( $battery + 0x100 ) % 0x100) / 10.0]
    } elseif {![string compare [string index $prefix 0] s]} {
      # sonar data packet
      if {[binary scan $data SSSSSSSSSSSSSSSS\
                     sonar(0)\
                     sonar(1)\
                     sonar(2)\
                     sonar(3)\
                     sonar(4)\
                     sonar(5)\
                     sonar(6)\
                     sonar(7)\
                     sonar(8)\
                     sonar(9)\
                     sonar(10)\
                     sonar(11)\
                     sonar(12)\
                     sonar(13)\
                     sonar(14)\
                     sonar(15)] != 16} {
        error "failed to get sonars"
      }
    } elseif {![string compare [string index $prefix 0] p]} {
      # position data packet
      if {[binary scan $data IIISSSSc\
                   time xpos ypos heading \
                   speed turnrate compass stall] != 8} {
        error "failed to get position data"
      }
    } elseif {![string compare [string index $prefix 0] l]} {
      set j 0
      while {$j < [expr $size / 2]} {
        if {[binary scan $data "x[expr 2 * $j]S" laser($j)] != 1} {
          error "failed to get laser scan $j"
        }
        set laser($j) [expr ($laser($j) + 0x10000) % 0x10000]
        incr j
      }
    } else {
      error "got unexpected message \"$data\""
    }
    incr i
  }
  return
}

proc writeCommand {device str} {
  global sock
  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }

  set size [binary format S [string length $str]]
  set cmd "c${device}${size}${str}"
  puts -nonewline $sock $cmd
  flush $sock
}

proc writeMotorCommand {fv tv} {
  set fvb [binary format S $fv]
  set tvb [binary format S $tv]

  writeCommand p "${fvb}${tvb}"
}
proc writeCameraCommand {p t z} {
  set pb [binary format S $p]
  set tb [binary format S $t]
  set zb [binary format S $z]

  #writeCommand v "${pb}${tb}${zb}"
  writeCommand z "${pb}${tb}${zb}"
}
  
proc printData {devices} {
  global frontbumpers rearbumpers sonar battery
  global time xpos ypos heading speed turnrate compass stall

  set i 0
  while {$i < [string length $devices]} {
    if {![string compare [string index $devices $i] m]} {
      puts "frontbumpers: $frontbumpers  rearbumpers: $rearbumpers battery:$battery"
    } elseif {![string compare [string index $devices $i] s]} {
      puts "Sonars: "
      parray sonar
    } elseif {![string compare [string index $devices $i] p]} {
      puts "Time: $time Pos: ($xpos,$ypos) Heading: $heading Speed: $speed Turnrate: $turnrate"
      puts "Compass: $compass Stall: $stall"
    } else {
      puts "Can't print unknown device \"[string index $devices $i]\""
    }
    incr i
  }
}

proc ChangeMotorState {state} {
  if {$state == 1} {
    requestFromRobot "xp[binary format S 2]m[binary format c 1]"
  } else {
    requestFromRobot "xp[binary format S 2]m[binary format c 0]"
  }
}

proc Stop {} {
  writeMotorCommand 0 0
}




      
 





  
  


