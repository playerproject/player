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
# Tcl client utilities for Player

#########################################################################
# Global constants and defaults necessary for Player
# 

# the default TCP port for Player
set PLAYER_DEFAULT_PORT 6665
set PLAYER_DEFAULT_HOST "localhost"

# the message start symbol
set PLAYER_STX 0x5878

# length of indentifier string that Player will send
set PLAYER_IDENT_STRLEN 32

# length of Player message header (minus the STX
set PLAYER_HEADER_LEN 30

# message types
set PLAYER_MSGTYPE_DATA 1
set PLAYER_MSGTYPE_CMD  2
set PLAYER_MSGTYPE_REQ  3
set PLAYER_MSGTYPE_RESP 4

# device codes
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
set PLAYER_SPEECH_CODE        12

# access modes
set PLAYER_READ_MODE   "r"
set PLAYER_WRITE_MODE  "w"
set PLAYER_ALL_MODE    "a"
set PLAYER_CLOSE_MODE  "c"

#
# Device-specific values
#

# the player device
set PLAYER_PLAYER_DEV_REQ      1
set PLAYER_PLAYER_DATA_REQ     2
set PLAYER_PLAYER_DATAMODE_REQ 3
set PLAYER_PLAYER_DATAFREQ_REQ 4

# the position device
set PLAYER_POSITION_MOTOR_POWER_REQ       1
set PLAYER_POSITION_VELOCITY_CONTROL_REQ  2
set PLAYER_POSITION_RESET_ODOM_REQ        3

# the vision device
set ACTS_NUM_CHANNELS 32
set ACTS_HEADER_SIZE [expr 2*$ACTS_NUM_CHANNELS]
set ACTS_BLOB_SIZE 10


# the sonar device
set PLAYER_NUM_SONAR_SAMPLES  16
set PLAYER_SONAR_POWER_REQ     4

# the laser device
set PLAYER_NUM_LASER_SAMPLES  401
set PLAYER_MAX_LASER_VALUE 8000

#########################################################################

#########################################################################
#
# initialize various data
#

if {![string length [info var player_port]]} {
  set player_port $PLAYER_DEFAULT_PORT
}
if {![string length [info var player_host]]} {
  set player_host $PLAYER_DEFAULT_HOST
}
if {![string length [info var player_sock]]} {
  set player_sock -1
}

# this array holds access modes for the devices that we've opened.
#    device_access($device,$index) = $access
array set device_access {}

#########################################################################
# data from Player will be stored in these vars

# the position device
set xpos 0 
set ypos 0
set heading 0
set speed 0 
set turnrate 0
set compass 0
set stall 0

# the sonar device
array set sonar {}

# the laser device
array set laser {}

# the vision device
array set vision {}

# the ptz device
set pan 0
set tilt 0
set zoom 0

# the misc device
set frontbumpers 0
set rearbumpers 0
set battery 0

# current time (latched from the most recent data packet)
set current_time_sec 0
set current_time_usec 0

#########################################################################

proc connectToRobot {robot} {
  global player_sock player_port player_host PLAYER_IDENT_STRLEN PLAYER_PLAYER_CODE \
         PLAYER_PLAYER_DATAMODE_REQ

  set player_sock [socket $robot $player_port]
  fconfigure $player_sock -blocking 1 -translation binary
  # print out who we're talking to
  puts "Connected to [read $player_sock $PLAYER_IDENT_STRLEN]"
  # make it request/reply
  requestFromRobot $PLAYER_PLAYER_CODE 0 "[binary format Sc $PLAYER_PLAYER_DATAMODE_REQ 1]"
  set player_host $robot
}

proc disconnectFromRobot {} {
  global player_sock
  if {$player_sock == -1} {
    error "ERROR: robot connection not set up"
  }
  close $player_sock
  set player_sock -1
}

proc requestFromRobot {device index req} {
  global player_sock PLAYER_STX device_access 
  global PLAYER_PLAYER_CODE PLAYER_PLAYER_CODE
  global PLAYER_MSGTYPE_REQ PLAYER_PLAYER_DEV_REQ
  global PLAYER_HEADER_LEN PLAYER_MSGTYPE_RESP
  global zero16 zero32 current_time_sec current_time_usec

  set size [string length $req]
  if {$player_sock == -1} {
    error "ERROR: robot connection not set up"
  }
  set isdevicerequest 0

  # write the request
  puts -nonewline $player_sock "[binary format SSSSIIIIII $PLAYER_STX $PLAYER_MSGTYPE_REQ $device $index 0 0 0 0 0 $size]${req}"
  flush $player_sock

  #
  # get the reply
  #
  while {1} {
     # wait for the STX
     while {1} {
       if {[binary scan [read $player_sock 2] S stx] != 1} {
         error "while waiting for STX"
       }
       if {$stx == $PLAYER_STX} {
         break
       }
     }

     set header [read $player_sock $PLAYER_HEADER_LEN]
     if {[binary scan $header SSSIIIIII \
                  type device device_index time_sec time_usec\
                  timestamp_sec timestamp_usec reserved size] != 9} {
      error "while scanning message header"
    }

    # update current time
    set current_time_sec $time_sec
    set current_time_usec $time_usec

    # is a reply?
    if {$type == $PLAYER_MSGTYPE_RESP} {
        break;
    } else {
      # if not, eat other data
      read $player_sock $size
    }
  }
  set reply [read $player_sock $size]

  #
  # if this is a device request, keep track of it accordingly
  #
  if {([binary scan $req S subtype] == 1) && \
      ($subtype == $PLAYER_PLAYER_DEV_REQ)} {
    set i 2
    while {$i < [string length $reply]} {
      if {[binary scan [string range $reply $i [expr $i + 4]] \
            SSa device index access] != 3} {
        error "Unable to determine device/index for bookkeeping"
      }
      set device_access($device,$index) $access
      incr i 5
    }

    if {![string compare $reply $req]} {
      #puts "- ok"
    } else {
      #puts "- but got \"$reply\" (length: [string length $reply])"
      puts "Error: bad reply \"$reply\""
    }
  }
}

#
# a helper to make device access requests easier
#
proc requestDeviceAccess {device index access} {
  global PLAYER_PLAYER_CODE PLAYER_PLAYER_DEV_REQ

  requestFromRobot $PLAYER_PLAYER_CODE 0 "[binary format SSSa $PLAYER_PLAYER_DEV_REQ $device $index $access]"
}

#
# this proc reads one entire round of data from the server, based on the 
# access modes listed in the 'device_access' array
# 
# resultant data will be stored in the appropriate global vars
#
proc readData {} {
  global player_sock device_access PLAYER_STX
  global PLAYER_READ_MODE PLAYER_ALL_MODE PLAYER_HEADER_LEN 
  global PLAYER_PLAYER_CODE PLAYER_PLAYER_DATA_REQ

  if {$player_sock == -1} {
    error "ERROR: robot connection not set up"
  }

  # request a data packet
  requestFromRobot $PLAYER_PLAYER_CODE 0 [binary format S $PLAYER_PLAYER_DATA_REQ]

  # how many devices are we reading from?
  set numdevices 0
  foreach dev [array names device_access] {
    if {![string compare $device_access($dev) $PLAYER_READ_MODE] ||
        ![string compare $device_access($dev) $PLAYER_ALL_MODE]} {
     incr numdevices
    }
  }

  set i 0
  while {$i < $numdevices} {
    # wait for the STX
    while {1} {
      if {[binary scan [read $player_sock 2] S stx] != 1} {
        error "while waiting for STX"
      }
      if {$stx == $PLAYER_STX} {
        break
      }
    }

    # read the rest of the header
    set header [read $player_sock $PLAYER_HEADER_LEN]
    if {[binary scan $header SSSIIIIII \
                  type device device_index time0 time1\
                  timestamp0 timestamp1 reserved size] != 9} {
      error "while scanning header"
    }

    # read the data
    set data [read $player_sock $size]
    if {[string length $data] != $size} {
      error "expected $size bytes, but only got [string length $data]"
    }
  
    # get the data out and put it in global vars
    parseData $device $device_index $data $size
    incr i
  }
}

# get the data out and put it in global vars
proc parseData {device device_index data size} {
  global battery frontbumpers rearbumpers
  global sonar
  global time xpos ypos heading speed turnrate compass stall
  global laser vision
  global pan tilt zoom 
  global ACTS_BLOB_SIZE ACTS_NUM_CHANNELS ACTS_HEADER_SIZE 
  global PLAYER_POSITION_CODE PLAYER_MISC_CODE
  global PLAYER_VISION_CODE PLAYER_LASER_CODE PLAYER_GRIPPER_CODE 
  global PLAYER_PTZ_CODE PLAYER_SONAR_CODE 
  global PLAYER_NUM_SONAR_SAMPLES PLAYER_NUM_LASER_SAMPLES
  global PLAYER_MAX_LASER_VALUE

  if {![string compare $device $PLAYER_VISION_CODE]} {
    # vision data packet
    set bufptr $ACTS_HEADER_SIZE
    set l 0
    while {$l < $ACTS_NUM_CHANNELS} {
      if {[binary scan $data "x[expr 2*$l+1]c" numblobs] != 1} {
        puts "Warning: failed to get number of blobs for channel $l"
        return
      }
      set vision($l,numblobs) [expr $numblobs - 1]
      set j 0
      while {$j < $vision($l,numblobs)} {
        if {[binary scan $data "x[expr $bufptr]cccccccccc" \
                  areabits(0) areabits(1) areabits(2) areabits(3)\
                  x y left right top bottom] != 10} {
          puts "Warning: failed to get blob info for ${j}th blob on ${l}th channel"
          return
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
      puts "Warning: failed to get pan/tilt/zoom"
      return
    }
  } elseif {![string compare $device $PLAYER_MISC_CODE]} {
    # misc data packet
    if {[binary scan $data ccc frontbumpers rearbumpers battery] != 3} {
      puts "Warning: failed to get bumpers and battery"
      return
    }
    # make them unsigned
    set frontbumpers [expr ( $frontbumpers + 0x100 ) % 0x100]
    set rearbumpers [expr ( $rearbumpers + 0x100 ) % 0x100]
    set battery [expr (( $battery + 0x100 ) % 0x100) / 10.0]
  } elseif {![string compare $device $PLAYER_SONAR_CODE]} {
    # sonar data packet
    if {[expr $size / 2] != $PLAYER_NUM_SONAR_SAMPLES} {
      puts "Warning: expected $PLAYER_NUM_SONAR_SAMPLES sonar readings, but received [expr $size/2] readings"
    }
    set j 0
    while {$j < [expr $size / 2]} {
      if {[binary scan $data "x[expr 2 * $j]S" sonar($j)] != 1} {
        puts "Warning: failed to get sonar scan $j"
        return
      }
      set sonar($j) [expr ($sonar($j) + 0x10000) % 0x10000]
      incr j
    }
  } elseif {![string compare $device $PLAYER_POSITION_CODE]} {
    # position data packet
    if {[binary scan $data IISSSSc\
                 xpos ypos heading \
                 speed turnrate compass stall] != 7} {
      puts "Warning: failed to get position data"
      return
    }
  } elseif {![string compare $device $PLAYER_LASER_CODE]} {
    if {[binary scan $data SSSS min_angle max_angle resolution \
                                 range_count] != 4} {
        puts "Warning: failed to parse laser header"
        return
    }

    set range_count [expr ($range_count + 0x10000) % 0x10000]

    if {[expr ($size-8) / 2] != $PLAYER_NUM_LASER_SAMPLES} {
      puts "Warning: expected $PLAYER_NUM_LASER_SAMPLES laser readings, but received [expr $size/2] readings"
    }
    set j 0
    while {$j < [expr $range_count]} {
      if {[binary scan $data "x8x[expr 2 * $j]S" laser($j)] != 1} {
        puts "Warning: failed to get laser scan $j"
        return
      }
      set laser($j) [expr ($laser($j) + 0x10000) % 0x10000]
      # TODO: why does stage return laser values of 11392?????
      if {$laser($j) > $PLAYER_MAX_LASER_VALUE} {
        #set laser($j) $PLAYER_MAX_LASER_VALUE
      }
      incr j
    }
  } else {
    puts "Warning: got unexpected message \"$data\""
    return
  }
}

proc writeCommand {device index str} {
  global player_sock PLAYER_STX PLAYER_MSGTYPE_CMD zero32
  if {$player_sock == -1} {
    error "ERROR: robot connection not set up"
  }

  set size [string length $str]
  set device_index [binary format S $index]
  #set cmd "c${device}${size}${str}"
  set cmd "[binary format SSSSIIIIII $PLAYER_STX $PLAYER_MSGTYPE_CMD $device $index 0 0 0 0 0 $size]${str}"
  puts -nonewline $player_sock $cmd
  flush $player_sock
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

proc writeSpeechCommand {str} {
  global PLAYER_SPEECH_CODE

  writeCommand $PLAYER_SPEECH_CODE 0 $str
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
  global PLAYER_POSITION_CODE PLAYER_POSITION_MOTOR_POWER_REQ
  if {$state == 1} {
    #requestFromRobot "xp[binary format S 2]m[binary format c 1]"
    requestFromRobot $PLAYER_POSITION_CODE 0 \
       "[binary format cc $PLAYER_POSITION_MOTOR_POWER_REQ 1]"
  } else {
    #requestFromRobot "xp[binary format S 2]m[binary format c 0]"
    requestFromRobot $PLAYER_POSITION_CODE 0 \
       "[binary format cc $PLAYER_POSITION_MOTOR_POWER_REQ 0]"
  }
}

proc ChangeSonarState {state} {
  global PLAYER_SONAR_CODE PLAYER_SONAR_POWER_REQ
  if {$state == 1} {
    requestFromRobot $PLAYER_SONAR_CODE 0 \
       "[binary format cc $PLAYER_SONAR_POWER_REQ 1]"
  } else {
    #requestFromRobot "xp[binary format S 2]m[binary format c 0]"
    requestFromRobot $PLAYER_SONAR_CODE 0 \
       "[binary format cc $PLAYER_SONAR_POWER_REQ 0]"
  }
}

proc Stop {} {
  writeMotorCommand 0 0
}




      
 





  
  


