#!/usr/bin/tclsh

#
# mktypedef.tcl
#
#    parses a saphira .p file to make typedef struct declarations
#

set USAGE "mktypedef.tcl \[files\]"
if {$argc < 1} {
  puts "$USAGE"
  exit
}


# read all the name/value pairs into an associative array, with new ones
# replacing old ones.  this way we'll get the union of all variable names
array set vars {}
set maxsonarnum 0
foreach fname $argv {

  set fd [open $fname r]
  
  set sonarnum 0
  while {![eof $fd]} {
    set line [gets $fd]
    if {![string length $line] || 
        ![string compare [string index $line 0] "@"] ||
        ![string compare [string index $line 0] "\;"] ||
        ![string compare [string index $line 0] " "] ||
        ![string compare [string index $line 0] "\["]} {
      continue
    }
    set name [lindex $line 0]
    if {![string compare $name "Section"]} { continue; }
    if {![string compare $name "SonarUnit"]} {incr sonarnum; continue; }
    set value [lindex $line 1]

    # If we determine the value to begin with ;, then the file doesn't give
    # a value (a apparently newly allowed syntatic construct in a .p file).
    # So we'll assume that an empty string is appropriate.
    if {![string compare [string index $value 0] "\;"]} {
      set value ""
    }

    set vars($name) $value
  }

  close $fd

  if {$sonarnum > $maxsonarnum} {set maxsonarnum $sonarnum}
}

# they took this one out for some reason, and only give the info in the manual
#set vars(Vel2Divisor) 20.0

puts "void initialize_robot_params(void);\n"
puts "#define PLAYER_NUM_ROBOT_TYPES [llength $argv]\n\n"

puts "typedef struct\n\{"
puts "  double x;"
puts "  double y;"
puts "  double th;"
puts "\} sonar_pose_t;\n\n"

puts "typedef struct\n\{"
# write each variable name out, inferring its type
foreach name [lsort [array names vars]] {

  set value $vars($name)

  if {![string compare $name Class] || 
      ![string compare $name LaserPort] ||
      ![string compare $name LaserIgnore] ||
      ![string compare $name Map] ||
      ![string compare $name Subclass]} {
    puts "  char* ${name};"
  } elseif {![string compare $value true] ||
            ![string compare $value false] ||
            ![string compare $value [expr round($value)]] ||
            ![string compare $name SonarNum]} {

    puts "  int $name; // [lrange $line 3 end]"
  } else {
    puts "  double $name; // [lrange $line 3 end]"
  }
}
puts "  sonar_pose_t sonar_pose\[$maxsonarnum\];"
puts "\} RobotParams_t;"

puts "\n\nextern RobotParams_t PlayerRobotParams\[\];"

