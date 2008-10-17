#!/usr/bin/tclsh

#
# parseparam.tcl
#
#    parses saphira .p file to get robot parameters out
#

set USAGE "parseparam.tcl <fname.p>"
if {$argc < 1} {
  puts "$USAGE"
  exit
}

set robotlist {}

# read all the name/value pairs into an associative array, with new ones
# replacing old ones.  this way we'll get the union of all variable names
array set vars {}
set maxsonarnum 0
set maxbumpernum 0
foreach fname $argv {

  set fd [open $fname r]
  
  set sonarnum 0
  set bumpernum 0
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
    if {![string compare $name "NumFrontBumpers"]} {incr bumpernum $value; }
    if {![string compare $name "NumRearBumpers"]} {incr bumpernum $value; }

    set vars($name) $value
  }

  close $fd

  if {$sonarnum > $maxsonarnum} {set maxsonarnum $sonarnum}
  if {$bumpernum > $maxbumpernum} {set maxbumpernum $bumpernum}
}

# they took this one out for some reason, and only give the info in the manual
#set vars(Vel2Divisor) 20.0

foreach fname $argv {
  set robottype [file rootname [file tail $fname]]
  # take out + and - characters, which will cause invalid C syntax
  set idx [string first "+" $robottype]
  while {$idx >= 0} {
    set robottype [string replace $robottype $idx $idx "plus"]
    set idx [string first "+" $robottype]
  }
  set idx [string first "-" $robottype]
  while {$idx >= 0} {
    set robottype [string replace $robottype $idx $idx "_"]
    set idx [string first "-" $robottype]
  }

  lappend robotlist $robottype
  set fd [open $fname r]

  catch {unset thisvars}
  while {![eof $fd]} {
    set line [gets $fd]

    if {![string length $line] || 
        ![string compare [string index $line 0] ";"] ||
        ![string compare [string index $line 0] " "]} {
      continue
    }
    set name [lindex $line 0]
    set value [lindex $line 1]

    if {![string compare $name "SonarUnit"]} {
      set thisvars(${name},$value) [lrange $line 2 end]
    } else {
      if {![string compare $value false]} {set value 0}
      if {![string compare $value true]} {set value 1}
      set thisvars($name) $value
    }
  }


  puts "\nRobotParams_t ${robottype}_params = \n\{"
  #if {![string compare [string range $thisvars(Subclass) 0 3] "pion"]} {
    #set thisvars(Vel2Divisor) 4.0
  #} else {
    #set thisvars(Vel2Divisor) 20.0
  #}

  
  foreach name [lsort [array names vars]] {

    if {[lsearch [array names thisvars] $name] < 0} {
      set value 0
    } else {
      set value $thisvars($name)
    }

    if {[string length $name]} {
      puts -nonewline "  "
      if {![string compare $name Class] || 
          ![string compare $name Subclass] ||
          ![string compare $name LaserPort] ||
          ![string compare $name Map] ||
          ![string compare $name LaserIgnore]} {
        puts -nonewline "\""
      }
      if {![string compare $value ";"]} {
        set value ""
      }
      puts -nonewline "$value"
      if {![string compare $name Class] || 
          ![string compare $name Subclass] ||
          ![string compare $name LaserPort] ||
          ![string compare $name Map] ||
          ![string compare $name LaserIgnore]} {
        puts -nonewline "\""
      }
      puts ","
      set name ""
    }
  }

  puts "  \{"
  foreach name [lsort -dictionary [array names thisvars "SonarUnit*"]] {
    puts "    \{ [join [split $thisvars($name)] ", "] \},"
  }
  #set i [llength [array names thisvars "SonarUnit*"]]
  #while {$i < $maxsonarnum} {
    #puts "    \{ 0, 0, 0 \},"
    #incr i
  #}
  puts "  \},"

  puts "  \{"
  set i 0
  while {$i < $maxbumpernum} {
    puts "    \{ 0, 0, 0, 0, 0 \},"
    incr i
  }
  puts "  \}"
  
  puts "\};"
}

puts "\nRobotParams_t PlayerRobotParams\[PLAYER_NUM_ROBOT_TYPES\];"
puts "\nvoid\ninitialize_robot_params(void)\n\{"
set i 0
foreach robottype $robotlist {
  puts "  PlayerRobotParams\[$i\] =  ${robottype}_params;"
  incr i
}
puts "\}"
