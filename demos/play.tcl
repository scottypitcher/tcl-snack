#!/bin/sh
# the next line restarts using tclsh \
exec tclsh8.3 "$0" "$@"

package require sound

if {[llength $argv] == 0} {
  puts {Usage: play.tcl file}
  exit
} else {
  set file [lindex $argv 0]
}

sound::sound s -file $file -debug 0
s play -block 1
