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
#

package provide Tclplayer 1.0

# we'll make variables from these names
set player_base_varname "pc"
set player_base_varnum 0

# the default TCP port for Player
set PLAYER_DEFAULT_PORT 6665
set PLAYER_DEFAULT_HOST "localhost"

# the message start symbol
set PLAYER_STX 0x5878
# length of indentifier string that Player will send
set PLAYER_IDENT_STRLEN 32
# length of Player message header (minus the STX)
set PLAYER_HEADER_LEN 30

# message types
set PLAYER_MSGTYPE_DATA 1
set PLAYER_MSGTYPE_CMD  2
set PLAYER_MSGTYPE_REQ  3
set PLAYER_MSGTYPE_RESP_ACK 4
set PLAYER_MSGTYPE_SYNCH 5
set PLAYER_MSGTYPE_RESP_NACK 6
set PLAYER_MSGTYPE_RESP_ERR 7

##############################
# the valid datamode codes 
##############################
# all data at fixed frequency
set PLAYER_DATAMODE_PUSH_ALL 0 
# all data on demand
set PLAYER_DATAMODE_PULL_ALL 1 
# only new new data at fixed freq
set PLAYER_DATAMODE_PUSH_NEW 2 
# only new data on demand
set PLAYER_DATAMODE_PULL_NEW 3 

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
set PLAYER_GPS_CODE           13
set PLAYER_BPS_CODE           16

set PLAYER_PLAYER_STRING         "player"
set PLAYER_MISC_STRING           "misc"
set PLAYER_GRIPPER_STRING        "gripper"
set PLAYER_POSITION_STRING       "position"
set PLAYER_SONAR_STRING          "sonar"
set PLAYER_LASER_STRING          "laser"
set PLAYER_VISION_STRING         "vision"
set PLAYER_PTZ_STRING            "ptz"
set PLAYER_AUDIO_STRING          "audio"
set PLAYER_LASERBEACON_STRING    "laserbeacon"
set PLAYER_BROADCAST_STRING      "broadcast"
set PLAYER_SPEECH_STRING         "speech"
set PLAYER_GPS_STRING            "gps"
set PLAYER_BPS_STRING            "bps"

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
set PLAYER_PLAYER_AUTH_REQ     5

# the position device
set PLAYER_POSITION_MOTOR_POWER_REQ       1
set PLAYER_POSITION_VELOCITY_CONTROL_REQ  2
set PLAYER_POSITION_RESET_ODOM_REQ        3

# the vision device
set VISION_NUM_CHANNELS 32
set VISION_HEADER_SIZE [expr 4*$VISION_NUM_CHANNELS]
set VISION_BLOB_SIZE 16

# the sonar device
set PLAYER_NUM_SONAR_SAMPLES  16
set PLAYER_SONAR_POWER_REQ    11

# the laser device
set PLAYER_NUM_LASER_SAMPLES  401
set PLAYER_MAX_LASER_VALUE 8000
set PLAYER_LASER_SET_CONFIG 1
set PLAYER_LASER_GET_CONFIG 2

set PLAYER_BPS_SUBTYPE_SETGAIN 1
set PLAYER_BPS_SUBTYPE_SETLASER 2
set PLAYER_BPS_SUBTYPE_SETBEACON 3

# the laserbeacon device
set PLAYER_LASERBEACON_SUBTYPE_SETCONFIG 1
set PLAYER_LASERBEACON_SUBTYPE_GETCONFIG 2

#########################################################################

#
# convert from codes and names
#
proc player_code_to_name {code} {
  global PLAYER_PLAYER_CODE PLAYER_MISC_CODE PLAYER_GRIPPER_CODE
  global PLAYER_POSITION_CODE PLAYER_SONAR_CODE PLAYER_LASER_CODE
  global PLAYER_VISION_CODE PLAYER_PTZ_CODE PLAYER_AUDIO_CODE
  global PLAYER_LASERBEACON_CODE PLAYER_BROADCAST_CODE PLAYER_SPEECH_CODE
  global PLAYER_GPS_CODE PLAYER_BPS_CODE

  global PLAYER_PLAYER_STRING PLAYER_MISC_STRING PLAYER_GRIPPER_STRING
  global PLAYER_POSITION_STRING PLAYER_SONAR_STRING PLAYER_LASER_STRING
  global PLAYER_VISION_STRING PLAYER_PTZ_STRING PLAYER_AUDIO_STRING
  global PLAYER_LASERBEACON_STRING PLAYER_BROADCAST_STRING PLAYER_SPEECH_STRING
  global PLAYER_GPS_STRING PLAYER_BPS_STRING

  if {$code == $PLAYER_PLAYER_CODE} {
    return $PLAYER_PLAYER_STRING
  } elseif {$code == $PLAYER_MISC_CODE} {
    return $PLAYER_MISC_STRING
  } elseif {$code == $PLAYER_GRIPPER_CODE} {
    return $PLAYER_GRIPPER_STRING
  } elseif {$code == $PLAYER_POSITION_CODE} {
    return $PLAYER_POSITION_STRING
  } elseif {$code == $PLAYER_SONAR_CODE} {
    return $PLAYER_SONAR_STRING
  } elseif {$code == $PLAYER_LASER_CODE} {
    return $PLAYER_LASER_STRING
  } elseif {$code == $PLAYER_VISION_CODE} {
    return $PLAYER_VISION_STRING
  } elseif {$code == $PLAYER_PTZ_CODE} {
    return $PLAYER_PTZ_STRING
  } elseif {$code == $PLAYER_AUDIO_CODE} {
    return $PLAYER_AUDIO_STRING
  } elseif {$code == $PLAYER_LASERBEACON_CODE} {
    return $PLAYER_LASERBEACON_STRING
  } elseif {$code == $PLAYER_BROADCAST_CODE} {
    return $PLAYER_BROADCAST_STRING
  } elseif {$code == $PLAYER_SPEECH_CODE} {
    return $PLAYER_SPEECH_STRING
  } elseif {$code == $PLAYER_GPS_CODE} {
    return $PLAYER_GPS_STRING
  } elseif {$code == $PLAYER_BPS_CODE} {
    return $PLAYER_BPS_STRING
  } else {
    return ""
  }
}

#
# convert string name to code.  also accepts code, which it won't modify
#
proc player_name_to_code {name} {
  global PLAYER_PLAYER_CODE PLAYER_MISC_CODE PLAYER_GRIPPER_CODE
  global PLAYER_POSITION_CODE PLAYER_SONAR_CODE PLAYER_LASER_CODE
  global PLAYER_VISION_CODE PLAYER_PTZ_CODE PLAYER_AUDIO_CODE
  global PLAYER_LASERBEACON_CODE PLAYER_BROADCAST_CODE PLAYER_SPEECH_CODE
  global PLAYER_GPS_CODE PLAYER_BPS_CODE

  global PLAYER_PLAYER_STRING PLAYER_MISC_STRING PLAYER_GRIPPER_STRING
  global PLAYER_POSITION_STRING PLAYER_SONAR_STRING PLAYER_LASER_STRING
  global PLAYER_VISION_STRING PLAYER_PTZ_STRING PLAYER_AUDIO_STRING
  global PLAYER_LASERBEACON_STRING PLAYER_BROADCAST_STRING PLAYER_SPEECH_STRING
  global PLAYER_GPS_STRING PLAYER_BPS_STRING

  # is it already an integer code?
  # (crude test...)
  if {![catch {expr $name + 1}]} {
    return $name
  }

  if {![string compare $name $PLAYER_PLAYER_STRING]} {
    return $PLAYER_PLAYER_CODE
  } elseif {![string compare $name $PLAYER_MISC_STRING]} {
    return $PLAYER_MISC_CODE
  } elseif {![string compare $name $PLAYER_GRIPPER_STRING]} {
    return $PLAYER_GRIPPER_CODE
  } elseif {![string compare $name $PLAYER_POSITION_STRING]} {
    return $PLAYER_POSITION_CODE
  } elseif {![string compare $name $PLAYER_SONAR_STRING]} {
    return $PLAYER_SONAR_CODE
  } elseif {![string compare $name $PLAYER_LASER_STRING]} {
    return $PLAYER_LASER_CODE
  } elseif {![string compare $name $PLAYER_VISION_STRING]} {
    return $PLAYER_VISION_CODE
  } elseif {![string compare $name $PLAYER_PTZ_STRING]} {
    return $PLAYER_PTZ_CODE
  } elseif {![string compare $name $PLAYER_AUDIO_STRING]} {
    return $PLAYER_AUDIO_CODE
  } elseif {![string compare $name $PLAYER_LASERBEACON_STRING]} {
    return $PLAYER_LASERBEACON_CODE
  } elseif {![string compare $name $PLAYER_BROADCAST_STRING]} {
    return $PLAYER_BROADCAST_CODE
  } elseif {![string compare $name $PLAYER_SPEECH_STRING]} {
    return $PLAYER_SPEECH_CODE
  } elseif {![string compare $name $PLAYER_GPS_STRING]} {
    return $PLAYER_GPS_CODE
  } elseif {![string compare $name $PLAYER_BPS_STRING]} {
    return $PLAYER_BPS_CODE
  } else {
    return 0
  }
}

#
# connect to the server
#
#  usage: player_connect [-reqrep] obj [host] [port]
#
# returns an object identifier for this client
proc player_connect {args} {
  global PLAYER_IDENT_STRLEN PLAYER_DATAMODE_PULL_ALL PLAYER_DATAMODE_PULL_NEW\
         PLAYER_PLAYER_DATAMODE_REQ PLAYER_DEFAULT_HOST \
         PLAYER_DEFAULT_PORT player_base_varname player_base_varnum

  set USAGE "Usage: player_connect \[-reqrep\] obj \[host\] \[port\]"
    
  set host $PLAYER_DEFAULT_HOST
  set port $PLAYER_DEFAULT_PORT
  set reqrep 0

  set i 0
  if {($i < [llength $args]) && ![string compare [lindex $args $i] "-reqrep"]} {
    set reqrep 1
    incr i
  }
  if {$i < [llength $args]} {
    set varname [lindex $args $i]
    incr i
  } else {
    error "No object specifed; $USAGE"
  }

  if {$i < [llength $args]} {
    set host [lindex $args $i]
    incr i
  }

  if {$i < [llength $args]} {
    set port [lindex $args $i]
    incr i
  }

  if {[llength [uplevel #0 info var $varname]]} {
    uplevel #0 unset $varname
  }
  upvar #0 $varname arr

  array set arr {}

  set sock [socket $host $port]
  fconfigure $sock -blocking 1 -translation binary -buffering none

  # print out who we're talking to
  puts "Connected to [read $sock $PLAYER_IDENT_STRLEN]"

  set arr(host) $host
  set arr(port) $port
  set arr(sock) $sock
  set arr(time_sec) 0
  set arr(time_usec) 0
  set arr(reqrep) $reqrep

  # make it request/reply
  if {$reqrep} {
    player_req $varname player 0 \
      "[binary format Sc $PLAYER_PLAYER_DATAMODE_REQ $PLAYER_DATAMODE_PULL_NEW]"
  }

  return $varname
}

# 
# disconnect from the server
#
proc player_disconnect {obj} {
  upvar #0 $obj arr

  if {![array exists arr]} {
    error "\"$obj\" isn't a client object, or isn't initialized"
  }

  close $arr(sock)
  set arr(sock) -1
}

#
# make a request of the server
#
# obj should be something returned by player_connect
#
# returns the reply body
#
proc player_req {obj device index req} {
  global PLAYER_STX PLAYER_MSGTYPE_REQ PLAYER_PLAYER_DEV_REQ
  global PLAYER_HEADER_LEN 
  global PLAYER_MSGTYPE_RESP_ACK
  global PLAYER_MSGTYPE_RESP_NACK
  global PLAYER_MSGTYPE_RESP_ERR

  upvar #0 $obj arr

  set device [player_name_to_code $device]

  if {![array exists arr]} {
    error "\"$obj\" isn't a client object, or isn't initialized"
  }
  set arr(request_pending) 1

  if {$arr(sock) == -1} {
    error "connection not set up"
  }

  set size [string length $req]

  # write the request
  puts -nonewline $arr(sock) "[binary format SSSSIIIIII $PLAYER_STX $PLAYER_MSGTYPE_REQ $device $index 0 0 0 0 0 $size]${req}"
  #flush $arr(sock)

  #
  # get the reply
  #
  while {1} {
    # wait for the STX
    while {1} {
      if {[binary scan [read $arr(sock) 2] S stx] != 1} {
        error "while waiting for STX"
      }
      if {$stx == $PLAYER_STX} {
        break
      }
      puts "got: $stx"
    }

    set header [read $arr(sock) $PLAYER_HEADER_LEN]
    if {[binary scan $header SSSIIIIII \
                 type device_rep index_rep arr(time_sec) arr(time_usec)\
                 timestamp_sec timestamp_usec reserved size] != 9} {
      error "scan failed on header"
    }

    # is it the reply we're looking for?
    if {(($type == $PLAYER_MSGTYPE_RESP_ACK) ||  \
         ($type == $PLAYER_MSGTYPE_RESP_ERR) ||  \
         ($type == $PLAYER_MSGTYPE_RESP_NACK)) && \
        $device_rep == $device && $index_rep == $index} {
        break;
    } else {
      # if not, eat other data
      read $arr(sock) $size
    }
  }

  if {$type == $PLAYER_MSGTYPE_RESP_NACK} {
    puts "WARNING: got NACK on request"
  } elseif {$type == $PLAYER_MSGTYPE_RESP_ERR} {
    puts "WARNING: got ERR on request"
  }
  
  set reply [read $arr(sock) $size]
  set arr(request_pending) 0
  return $reply
}

#
# request access to a device. 
#
# obj should be something returned by player_connect
#
# returns access that was granted
proc player_req_dev {obj device access {index 0}} {
  global PLAYER_PLAYER_DEV_REQ
  upvar #0 $obj arr

  # first get the code
  set code [player_name_to_code $device]
  set req "[binary format SSSa $PLAYER_PLAYER_DEV_REQ $code $index $access]"
  set rep [player_req $obj player 0 $req]
  if {[binary scan $rep SSSa ioctl_rep device_rep index_rep access_rep] != 4} { 
    error "scan failed on reply"
  }

  # now get the name
  set name [player_code_to_name $code]
  set arr($name,$index,access) $access_rep
  return $access_rep
}

proc player_read {obj} {
  global PLAYER_STX PLAYER_READ_MODE PLAYER_ALL_MODE PLAYER_HEADER_LEN 
  global PLAYER_PLAYER_DATA_REQ PLAYER_MSGTYPE_SYNCH PLAYER_MSGTYPE_DATA

  upvar #0 $obj arr

  if {![array exists arr]} {
    error "\"$obj\" isn't a client object, or isn't initialized"
  }
  if {$arr(sock) == -1} {
    error "connection not set up"
  }

  if {$arr(request_pending)} {
    return
  }

  # request a data packet
  if {$arr(reqrep)} {
    player_req $obj player 0 \
      [binary format S $PLAYER_PLAYER_DATA_REQ]
  }

  # read until we hit the SYNCH packet
  while {1} {

    # wait for the STX
    while {1} {
      if {[binary scan [read $arr(sock) 2] S stx] != 1} {
        error "while waiting for STX"
      }
      if {$stx == $PLAYER_STX} {
        break
      }
    }

    # read the rest of the header

    #puts "reading header"
    set header [read $arr(sock) $PLAYER_HEADER_LEN]
    if {[binary scan $header SSSIIIIII \
                  type device device_index time0 time1\
                  timestamp0 timestamp1 reserved size] != 9} {
      error "scan failed on header"
    }

    # read the data
    set data [read $arr(sock) $size]
    if {[string length $data] != $size} {
      error "expected $size bytes, but only got [string length $data]"
    }

    # if we get the SYNCH packet, then stop reading
    if {$type == $PLAYER_MSGTYPE_SYNCH} {
      break
    } elseif {$type == $PLAYER_MSGTYPE_DATA} {
      # get the data out and put it in global vars
      player_parse_data $obj $device $device_index $data $size
    } else {
      error "player_read: received packet of unexpected type: $type"
    }
  }
}

# get the data out and put it in arr vars
proc player_parse_data {obj device device_index data size} {
  global VISION_BLOB_SIZE VISION_NUM_CHANNELS VISION_HEADER_SIZE 
  global PLAYER_MISC_CODE PLAYER_GRIPPER_CODE
  global PLAYER_POSITION_CODE PLAYER_SONAR_CODE PLAYER_LASER_CODE
  global PLAYER_VISION_CODE PLAYER_PTZ_CODE PLAYER_AUDIO_CODE
  global PLAYER_LASERBEACON_CODE PLAYER_BROADCAST_CODE PLAYER_SPEECH_CODE
  global PLAYER_GPS_CODE PLAYER_BPS_CODE
  global PLAYER_NUM_SONAR_SAMPLES PLAYER_NUM_LASER_SAMPLES

  upvar #0 $obj arr

  if {![array exists arr]} {
    error "\"$obj\" isn't a client object, or isn't initialized"
  }

  set name [player_code_to_name $device]

  if {$device == $PLAYER_MISC_CODE} {
    # misc data packet
    if {[binary scan $data ccccc \
            arr($name,$device_index,frontbumpers) \
            arr($name,$device_index,rearbumpers) \
            arr($name,$device_index,battery) \
            arr($name,$device_index,analog) \
            arr($name,$device_index,digin)] != 5} {
      puts "Warning: failed to get bumpers and battery"
      return
    }
    # make them unsigned
    set arr($name,$device_index,frontbumpers) \
      [expr $arr($name,$device_index,frontbumpers) & 0xFF]
    set arr($name,$device_index,rearbumpers) \
      [expr $arr($name,$device_index,rearbumpers) & 0xFF]
    set arr($name,$device_index,battery) \
      [expr ($arr($name,$device_index,battery) & 0xFF) / 10.0]
    set arr($name,$device_index,analog) \
      [expr $arr($name,$device_index,analog) & 0xFF]
    set arr($name,$device_index,digin) \
      [expr $arr($name,$device_index,digin) & 0xFF]

    # copy into non-indexed spot for convenience
    if {!$device_index} {
      set arr($name,frontbumpers) $arr($name,$device_index,frontbumpers)
      set arr($name,rearbumpers) $arr($name,$device_index,rearbumpers)
      set arr($name,battery) $arr($name,$device_index,battery)
      set arr($name,analog) $arr($name,$device_index,analog)
      set arr($name,digin) $arr($name,$device_index,digin)
    }
  } elseif {$device == $PLAYER_GRIPPER_CODE} {
    # ptz data packet
    if {[binary scan $data cc \
             arr($name,$device_index,byte0)  \
             arr($name,$device_index,byte1)] != 2} {
      puts "Warning: failed to get gripper bytes"
      return
    }

    # make them unsigned
    set arr($name,$device_index,byte0) \
           [expr $arr($name,$device_index,byte0) & 0xFF]
    set arr($name,$device_index,byte1) \
           [expr $arr($name,$device_index,byte1) & 0xFF]

    # copy into non-indexed spot for convenience
    if {!$device_index} {
      set arr($name,byte0) $arr($name,$device_index,byte0)
      set arr($name,byte1) $arr($name,$device_index,byte1)
    }
  } elseif {$device == $PLAYER_POSITION_CODE} {
    # position data packet
    if {[binary scan $data IISSSSSc \
                 arr($name,$device_index,xpos) \
                 arr($name,$device_index,ypos) \
                 arr($name,$device_index,heading) \
                 arr($name,$device_index,speed) \
                 arr($name,$device_index,sidespeed) \
                 arr($name,$device_index,turnrate) \
                 arr($name,$device_index,compass) \
                 arr($name,$device_index,stall)] != 8} {
      puts "Warning: failed to get position data"
      return
    }
    # make them unsigned
    set arr($name,$device_index,heading) \
      [expr $arr($name,$device_index,heading) & 0xFFFF]
    set arr($name,$device_index,compass) \
      [expr $arr($name,$device_index,compass) & 0xFFFF]
    set arr($name,$device_index,stall) \
      [expr $arr($name,$device_index,stall) & 0xFF]

    # copy into non-indexed spot for convenience
    if {!$device_index} {
      set arr($name,xpos) $arr($name,$device_index,xpos)
      set arr($name,ypos) $arr($name,$device_index,ypos)
      set arr($name,heading) $arr($name,$device_index,heading)
      set arr($name,speed) $arr($name,$device_index,speed)
      set arr($name,sidespeed) $arr($name,$device_index,sidespeed)
      set arr($name,turnrate) $arr($name,$device_index,turnrate)
      set arr($name,compass) $arr($name,$device_index,compass)
      set arr($name,stall) $arr($name,$device_index,stall)
    }
  } elseif {$device == $PLAYER_SONAR_CODE} {
    # sonar data packet
    if {[expr $size / 2] != $PLAYER_NUM_SONAR_SAMPLES} {
      puts "Warning: expected $PLAYER_NUM_SONAR_SAMPLES sonar readings, but received [expr $size/2] readings"
    }
    set j 0
    while {$j < [expr $size / 2]} {
      if {[binary scan $data "x[expr 2 * $j]S" \
               arr($name,$device_index,$j)] != 1} {
        puts "Warning: failed to get sonar scan $j"
        return
      }

      # make it unsigned
      set arr($name,$device_index,$j) \
        [expr $arr($name,$device_index,$j) & 0xFFFF]
      # copy into non-indexed spot for convenience
      if {!$device_index} {
        set arr($name,$j) $arr($name,$device_index,$j)
      }
      incr j
    }
  } elseif {$device == $PLAYER_LASER_CODE} {

    if {[binary scan $data SSSS \
           arr($name,$device_index,min_angle) \
           arr($name,$device_index,max_angle) \
           arr($name,$device_index,resolution) \
           arr($name,$device_index,range_count)] != 4} {
        puts "Warning: failed to parse laser header"
        #puts "$arr($name,$device_index,min_angle) $arr($name,$device_index,max_angle) $arr($name,$device_index,resolution) $arr($name,$device_index,range_count)"
        puts "[binary scan $data SSSS arr($name,$device_index,min_angle) arr($name,$device_index,max_angle) arr($name,$device_index,resolution) arr($name,$device_index,range_count)]"
        return
    }

    # make it unsigned
    set arr($name,$device_index,range_count) \
      [expr $arr($name,$device_index,range_count) & 0xFFFF]

    if {[expr ($size-8) / 2] != $PLAYER_NUM_LASER_SAMPLES} {
      puts "Warning: expected $PLAYER_NUM_LASER_SAMPLES laser readings, but received [expr $size/2] readings"
    }
    #puts "laser: $arr($name,$device_index,min_angle), $arr($name,$device_index,max_angle), $arr($name,$device_index,resolution), $arr($name,$device_index,range_count)"
    set j 0
    while {$j < $arr($name,$device_index,range_count)} {
      if {[binary scan $data "x8x[expr 2 * $j]S" \
                arr($name,$device_index,$j)] != 1} {
        puts "Warning: failed to get laser scan $j"
        return
      }
      # make it unsigned (and only use the lower 13 bits)
      set arr($name,$device_index,$j) \
        [expr $arr($name,$device_index,$j) & 0x1FFF]
      # copy into non-indexed spot for convenience
      if {!$device_index} {
        set arr($name,$j) $arr($name,$device_index,$j)
        set arr($name,min_angle) $arr($name,$device_index,min_angle) 
        set arr($name,max_angle) $arr($name,$device_index,max_angle)
        set arr($name,resolution) $arr($name,$device_index,resolution)
        set arr($name,range_count) $arr($name,$device_index,range_count)
      }
      incr j
    }
  } elseif {$device == $PLAYER_VISION_CODE} {

    set bufptr $VISION_HEADER_SIZE
    set l 0
    while {$l < $VISION_NUM_CHANNELS} {
      if {[binary scan $data "x[expr 4*$l+2]S" numblobs] != 1} {
        puts "Warning: failed to get number of blobs for channel $l"
        return
      }
      #puts "looking for $numblobs blobs on channel $l"
      
      set arr($name,$device_index,$l,numblobs) $numblobs
      if {!$device_index} {
        set arr($name,$l,numblobs) $arr($name,$device_index,$l,numblobs)
      }
      set j 0
      while {$j < $arr($name,$device_index,$l,numblobs)} {
        if {[binary scan $data "x${bufptr}IISSSSSS" \
                  color area x y left right top bottom] != 8} {
          puts "Warning: failed to get blob info for ${j}th blob on ${l}th channel"
          return
        }
        # make everything unsigned
        #set area [expr $area & 0xFFFFFFFF]
        set x [expr $x & 0xFFFF]
        set y [expr $y & 0xFFFF]
        set left [expr $left & 0xFFFF]
        set right [expr $right & 0xFFFF]
        set top [expr $top & 0xFFFF]
        set bottom [expr $bottom & 0xFFFF]

        set arr($name,$device_index,$l,$j,color) $color
        set arr($name,$device_index,$l,$j,area) $area
        set arr($name,$device_index,$l,$j,x) $x
        set arr($name,$device_index,$l,$j,y) $y
        set arr($name,$device_index,$l,$j,left) $left
        set arr($name,$device_index,$l,$j,right) $right
        set arr($name,$device_index,$l,$j,top) $top
        set arr($name,$device_index,$l,$j,bottom) $bottom

        if {!$device_index} {
          set arr($name,$l,$j,color) $arr($name,$device_index,$l,$j,color)
          set arr($name,$l,$j,area) $arr($name,$device_index,$l,$j,area)
          set arr($name,$l,$j,x) $arr($name,$device_index,$l,$j,x)
          set arr($name,$l,$j,y) $arr($name,$device_index,$l,$j,y)
          set arr($name,$l,$j,left) $arr($name,$device_index,$l,$j,left)
          set arr($name,$l,$j,right) $arr($name,$device_index,$l,$j,right)
          set arr($name,$l,$j,top) $arr($name,$device_index,$l,$j,top)
          set arr($name,$l,$j,bottom) $arr($name,$device_index,$l,$j,bottom)
        }

        incr bufptr $VISION_BLOB_SIZE
        incr j
      }
      incr l
    }
  } elseif {$device == $PLAYER_PTZ_CODE} {
    # ptz data packet
    if {[binary scan $data SSS \
             arr($name,$device_index,pan)  \
             arr($name,$device_index,tilt) \
             arr($name,$device_index,zoom)] != 3} {
      puts "Warning: failed to get pan/tilt/zoom"
      return
    }
    # make them unsigned
    set arr($name,$device_index,zoom) \
      [expr $arr($name,$device_index,zoom) & 0xFFFF]
    if {!$device_index} {
      set arr($name,pan) $arr($name,$device_index,pan)
      set arr($name,tilt) $arr($name,$device_index,tilt)
      set arr($name,zoom) $arr($name,$device_index,zoom)
    }
  } elseif {$device == $PLAYER_AUDIO_CODE} {
    # audio goes here...
  } elseif {$device == $PLAYER_LASERBEACON_CODE} {
    # kill the old ones
    if {[info exists arr($name,$device_index,numbeacons)]} {
      set i 0
      while {$i < $arr($name,$device_index,numbeacons)} {
        unset arr($name,$device_index,$i,id)
        unset arr($name,$device_index,$i,range)
        unset arr($name,$device_index,$i,bearing)
        unset arr($name,$device_index,$i,orient)
        if {!$device_index} {
          unset arr($name,$i,id)
          unset arr($name,$i,range)
          unset arr($name,$i,bearing)
          unset arr($name,$i,orient)
        }
        incr i
      }
    }
    # first get the count
    if {[binary scan $data S arr($name,$device_index,numbeacons)] != 1} {
      puts "Warning: failed to get laserbeacon count"
      return
    }
    if {!$device_index} {
      set arr($name,numbeacons) $arr($name,$device_index,numbeacons)
    }
    set i 0
    while {$i < $arr($name,$device_index,numbeacons)} {
      if {[binary scan $data x[expr 2+($i*7)]cSSS \
             arr($name,$device_index,$i,id) \
             arr($name,$device_index,$i,range) \
             arr($name,$device_index,$i,bearing) \
             arr($name,$device_index,$i,orient)] != 4} {
        puts "Warning unable to get info for beacon $i"
        return
      }

      # make some unsigned
      set arr($name,$device_index,$i,id) \
        [expr $arr($name,$device_index,$i,id) & 0xFF]
      set arr($name,$device_index,$i,range) \
        [expr $arr($name,$device_index,$i,range) & 0xFFFF]

      if {!$device_index} {
        set arr($name,$i,id) $arr($name,$device_index,$i,id)
        set arr($name,$i,range) $arr($name,$device_index,$i,range)
        set arr($name,$i,bearing) $arr($name,$device_index,$i,bearing)
        set arr($name,$i,orient) $arr($name,$device_index,$i,orient)
      }
      incr i
    }
  } elseif {$device == $PLAYER_BROADCAST_CODE} {
    # broadcast goes here...
  } elseif {$device == $PLAYER_SPEECH_CODE} {
    # speech goes here...
  } elseif {$device == $PLAYER_GPS_CODE} {
    if {[binary scan $data III \
          arr($name,$device_index,x)\
          arr($name,$device_index,y)\
          arr($name,$device_index,heading)] != 3} {
      puts "Warning: failed to get gps X,Y,theta"
      return
    }
    if {!$device_index} {
      set arr($name,x) $arr($name,$device_index,x)
      set arr($name,y) $arr($name,$device_index,y)
      set arr($name,heading) $arr($name,$device_index,heading)
    }
  } elseif {$device == $PLAYER_BPS_CODE} {
    if {[binary scan $data IIIIIII \
          arr($name,$device_index,px)\
          arr($name,$device_index,py)\
          arr($name,$device_index,pa)\
          arr($name,$device_index,ux)\
          arr($name,$device_index,uy)\
          arr($name,$device_index,ua)\
          arr($name,$device_index,err)] != 7} {
      puts "Warning: failed to get bps data"
      return
    }
    if {!$device_index} {
      set arr($name,px) $arr($name,$device_index,px)
      set arr($name,py) $arr($name,$device_index,py)
      set arr($name,pa) $arr($name,$device_index,pa)
      set arr($name,ux) $arr($name,$device_index,ux)
      set arr($name,uy) $arr($name,$device_index,uy)
      set arr($name,ua) $arr($name,$device_index,ua)
      set arr($name,err) $arr($name,$device_index,err)
    }
  } else {
    puts "Warning: got unexpected message \"$data\""
  }
}

proc player_write {obj device index str} {
  global PLAYER_STX PLAYER_MSGTYPE_CMD

  upvar #0 $obj arr

  set code [player_name_to_code $device]
  if {![array exists arr]} {
    error "\"$obj\" isn't a client object, or isn't initialized"
  }
  if {$arr(sock) == -1} {
    error "connection not set up"
  }

  set size [string length $str]
  set cmd "[binary format SSSSIIIIII $PLAYER_STX $PLAYER_MSGTYPE_CMD $code $index 0 0 0 0 0 $size]${str}"
  puts -nonewline $arr(sock) $cmd
  #flush $arr(sock)
}


# NOTE: this one sets the 2nd position command field to zero
proc player_set_speed {obj fv tv {index 0}} {
  set fvb [binary format S [expr round($fv)]]
  set svb [binary format S 0]
  set tvb [binary format S [expr round($tv)]]

  player_write $obj position $index "${fvb}${svb}${tvb}"
}

proc player_set_camera {obj p t z {index 0}} {
  set pb [binary format S [expr round($p)]]
  set tb [binary format S [expr round($t)]]
  set zb [binary format S [expr round($z)]]

  player_write $obj ptz $index "${pb}${tb}${zb}"
}

proc player_authenticate {obj key} {
  global PLAYER_PLAYER_AUTH_REQ
  player_req $obj player 0 \
       "[binary format S $PLAYER_PLAYER_AUTH_REQ]$key"
  return
}

proc player_change_motor_state {obj state {index 0}} {
  global PLAYER_POSITION_MOTOR_POWER_REQ
  if {$state == 1} {
    player_req $obj position $index \
       "[binary format cc $PLAYER_POSITION_MOTOR_POWER_REQ 1]"
  } else {
    player_req $obj position $index \
       "[binary format cc $PLAYER_POSITION_MOTOR_POWER_REQ 0]"
  }
  return
}
proc player_change_sonar_state {obj state {index 0}} {
  global PLAYER_SONAR_POWER_REQ
  if {$state == 1} {
    player_req $obj sonar $index \
       "[binary format cc $PLAYER_SONAR_POWER_REQ 1]"
  } else {
    player_req $obj sonar $index \
       "[binary format cc $PLAYER_SONAR_POWER_REQ 0]"
  }
  return
}

proc player_config_laser {obj min_angle max_angle resolution \
                          intensity {index 0}} {
  global PLAYER_LASER_SET_CONFIG

  player_req $obj laser $index \
       "[binary format cSSSc $PLAYER_LASER_SET_CONFIG $min_angle $max_angle $resolution $intensity]"
  return
}

proc player_config_laserbeacon {obj bit_count bit_size zero_thresh \
                                one_thresh {index 0}} {
  global PLAYER_LASERBEACON_SUBTYPE_GETCONFIG
  global PLAYER_LASERBEACON_SUBTYPE_SETCONFIG

  player_req $obj laserbeacon $index \
       "[binary format ccSSS $PLAYER_LASERBEACON_SUBTYPE_SETCONFIG \
                             $bit_count $bit_size $zero_thresh $one_thresh]"
  return
}

proc player_set_bps_beacon {obj id px py pa {index 0}} {
  global PLAYER_BPS_SUBTYPE_SETBEACON
  player_req $obj bps $index \
       "[binary format ccIIIIII $PLAYER_BPS_SUBTYPE_SETBEACON \
                                $id $px $py $pa 0 0 0]"
  return
}

proc player_set_laserbeacon_bits {obj bit_count {index 0}} {
  player_config_laserbeacon $obj $bit_count 50 10 95 $index
  return
}

proc player_stop_robot {obj} {
  player_set_speed $obj 0 0
}

