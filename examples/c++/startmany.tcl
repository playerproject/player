#!/usr/bin/tclsh

#
# helper script for starting lots of the same program (useful with Stage)
#

set USAGE "startmany.tcl <prog> <num> \[<host>\] \[<baseport>\]"

if {$argc < 2} {
  puts $USAGE
  exit
}

set prog [lindex $argv 0]
set num [lindex $argv 1]

set host "localhost"
set baseport 6665
if {$argc > 2} {
  set host [lindex $argv 2]
}
if {$argc > 3} {
  set baseport [lindex $argv 3]
}

puts "Starting $num instances of $prog on ${host}:${baseport}"

set i 0
while {$i < $num} {
  exec $prog -h $host -p [expr $baseport + $i] &
  incr i
}
