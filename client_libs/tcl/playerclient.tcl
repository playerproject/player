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
array set devices {}
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

set PLAYER_IDENT_STRLEN 32
set PLAYER_HEADER_LEN 32

set PLAYER_MSGTYPE_DATA 1
set PLAYER_MSGTYPE_CMD  2
set PLAYER_MSGTYPE_REQ  3
set PLAYER_MSGTYPE_RESP 4

set PLAYER_PLAYER_CODE         1
set PLAYER_MISC_CODE           2
set PLAYER_GRIPPER_CODE        3
set PLAYER_POSITION_CODE       4
set PLAYER_SONAR_CODE          5
set PLAYER_LASER_CODE          6
set PLAYER_VISION_CODE         7
set PLAYER_PTZ_CODE            8
set PLAYER_AUDIO_CODE          9
set PLAYER_LASERBEACON_CODE   10
set PLAYER_BROADCAST_CODE     11

set PLAYER_STX "xX" 

set PLAYER_PLAYER_DEV_REQ      1
set PLAYER_PLAYER_DATA_REQ     2
set PLAYER_PLAYER_DATAMODE_REQ 3
set PLAYER_PLAYER_DATAFREQ_REQ 4

set zero16 [binary format S 0]
set zero32 [binary format I 0]


proc connectToRobot {robot} {
  global sock port host PLAYER_IDENT_STRLEN

  set sock [socket $robot $port]
  fconfigure $sock -blocking 1 -translation binary
  # print out who we're talking to
  puts "Connected to [read $sock $PLAYER_IDENT_STRLEN]"
  # make it request/reply
  requestFromRobot "dm[binary format c 1]"
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
  global sock host PLAYER_STX PLAYER_PLAYER_CODE PLAYER_PLAYER_CODE \
         PLAYER_MSGTYPE_REQ devices PLAYER_PLAYER_DEV_REQ \
         zero16 zero32 PLAYER_HEADER_LEN PLAYER_MSGTYPE_RESP

  set size [binary format I [string length $req]]
  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }
  set isdevicerequest 0

  #puts "Requesting \"$req\" (length: [string length $req])"

  puts -nonewline $sock "${PLAYER_STX}${PLAYER_MSGTYPE_REQ}${PLAYER_PLAYER_CODE}${zero16}${zero32}${zero32}${zero32}${zero32}${zero32}${size}${req}"
  flush $sock

  #
  # get the reply
  #
  while {1} {
     set header [read $sock $PLAYER_HEADER_LEN]
     if {[binary scan $header a2SSSIIIIII \
                  stx type device device_index time0 time1\
                  timestamp0 timestamp1 reserved size] != 10} {
      error "scanning header"
    }
    #puts "stx: $stx"
    #puts "type: $type"
    #puts "device: $device"
    #puts "device_index: $device_index"
    #puts "time0: $time0"
    #puts "time1: $time1"
    #puts "timestamp0: $timestamp0"
    #puts "timestamp1: $timestamp1"
    #puts "reserved: $reserved"
    #puts "size: $size"
    if {![string compare $type $PLAYER_MSGTYPE_RESP]} {
        break;
    }
    # eat other data
    read $sock $size
  }
  set reply [read $sock $size]

  #
  # if this is a device request, keep track of it accordingly
  #
  if {![string compare [string range $req 0 1] $PLAYER_PLAYER_DEV_REQ]} {
    set i 2
    while {$i < [string length $reply]} {
      if {[binary scan [string range $reply [expr $i + 2] [expr $i + 3]] \
            S index] != 1} {
        error "Unable to determine device index for bookkeeping"
      }
      set devices([string range $reply $i [expr $i +1]],$index) \
            [string index $reply [expr $i + 4]]
      set i [expr $i + 5]
    }

    if {![string compare $reply $req]} {
      #puts "- ok"
    } else {
      #puts "- but got \"$reply\" (length: [string length $reply])"
      error "bad reply \"$reply\""
    }
  }
}

proc readData {} {
  global sock devices
  global battery frontbumpers rearbumpers
  global sonar
  global time xpos ypos heading speed turnrate compass stall
  global laser vision
  global pan tilt zoom
  global ACTS_BLOB_SIZE ACTS_NUM_CHANNELS ACTS_HEADER_SIZE 
  global PLAYER_HEADER_LEN PLAYER_POSITION_CODE PLAYER_MISC_CODE
  global PLAYER_VISION_CODE PLAYER_LASER_CODE PLAYER_GRIPPER_CODE 
  global PLAYER_PTZ_CODE PLAYER_SONAR_CODE

  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }

  set i 0
  # request a packet
  requestFromRobot "dp"
  # how many devices are we reading from?
  set numdevices 0
  foreach dev [array names devices] {
    if {![string compare $devices($dev) "r"] ||
        ![string compare $devices($dev) "a"]} {
     incr numdevices
    }
  }

  while {$i < $numdevices} {
    set header [read $sock $PLAYER_HEADER_LEN]
    if {[binary scan $header a2a2a2SIIIIII \
                  stx type device device_index time0 time1\
                  timestamp0 timestamp1 reserved size] != 10} {
      error "scanning header"
    }

    #puts "stx: $stx"
    #puts "type: $type"
    #puts "device: $device"
    #puts "device_index: $device_index"
    #puts "time0: $time0"
    #puts "time1: $time1"
    #puts "timestamp0: $timestamp0"
    #puts "timestamp1: $timestamp1"
    #puts "reserved: $reserved"
    #puts "size: $size"
  
    set data [read $sock $size]
    if {[string length $data] != $size} {
      error "expected $size bytes, but only got [string length $data]"
    }
  
    if {![string compare $device $PLAYER_VISION_CODE]} {
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
    } elseif {![string compare $device $PLAYER_PTZ_CODE]} {
      # ptz data packet
      if {[binary scan $data SSS pan tilt zoom] != 3} {
        error "failed to get pan/tilt/zoom"
      }
    } elseif {![string compare $device $PLAYER_MISC_CODE]} {
      # misc data packet
      if {[binary scan $data ccc frontbumpers rearbumpers battery] != 3} {
        error "failed to get bumpers and battery"
      }
      # make them unsigned
      set frontbumpers [expr ( $frontbumpers + 0x100 ) % 0x100]
      set rearbumpers [expr ( $rearbumpers + 0x100 ) % 0x100]
      set battery [expr (( $battery + 0x100 ) % 0x100) / 10.0]
    } elseif {![string compare $device $PLAYER_SONAR_CODE]} {
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
    } elseif {![string compare $device $PLAYER_POSITION_CODE]} {
      # position data packet
      if {[binary scan $data IISSSSc\
                   xpos ypos heading \
                   speed turnrate compass stall] != 7} {
        error "failed to get position data"
      }
    } elseif {![string compare $device $PLAYER_LASER_CODE]} {
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

proc writeCommand {device index str} {
  global sock PLAYER_STX PLAYER_MSGTYPE_CMD zero32
  if {$sock == -1} {
    error "ERROR: robot connection not set up"
  }

  set size [binary format I [string length $str]]
  set device_index [binary format S $index]
  #set cmd "c${device}${size}${str}"
  set cmd "${PLAYER_STX}${PLAYER_MSGTYPE_CMD}${device}${device_index}${zero32}${zero32}${zero32}${zero32}${zero32}${size}${str}"
  puts -nonewline $sock $cmd
  flush $sock
}

proc writeMotorCommand {fv tv} {
  global PLAYER_POSITION_CODE
  set fvb [binary format S $fv]
  set tvb [binary format S $tv]

  writeCommand $PLAYER_POSITION_CODE 0 "${fvb}${tvb}"
}
proc writeCameraCommand {p t z} {
  global PLAYER_PTZ_CODE

  set pb [binary format S $p]
  set tb [binary format S $t]
  set zb [binary format S $z]

  writeCommand $PLAYER_PTZ_CODE 0 "${pb}${tb}${zb}"
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
    #requestFromRobot "xp[binary format S 2]m[binary format c 1]"
  } else {
    #requestFromRobot "xp[binary format S 2]m[binary format c 0]"
  }
}

proc Stop {} {
  writeMotorCommand 0 0
}




      
 





  
  


