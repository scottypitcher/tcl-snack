#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

# A cross-platform mixer application that adapts to the capabilities
# of Snack on the machine it is run on.
# Lots of functionality on Linux - play volume only on Windows, currently.

package require -exact snack 1.6

proc Update {} {
  global doMonitor
  audio update
  if $doMonitor { after 100 Update }
}
bind . <Configure> Update

pack [frame .f] -expand yes -fill both
pack [checkbutton .r -text Monitor -command Update -variable doMonitor]

foreach jack [audio mixers] {
  pack [frame .f.g$jack -bd 1 -relief solid] -side left -expand yes -fill both
  pack [label .f.g$jack.l -text $jack]
  if {[audio channels $jack] == "Mono"} {
    audio volume $jack v(r$jack)
  } else {
    audio volume $jack v(l$jack) v(r$jack)
    pack [scale .f.g$jack.e -from 100 -to 0 -show no -var v(l$jack)] -side \
	    left -expand yes -fill both
  }
  pack [scale .f.g$jack.s -from 100 -to 0 -show no -var v(r$jack)] -expand yes\
	  -fill both
}

pack [frame .f.f2] -side left

if {[llength [audio inputs]] > 0} {
  pack [label .f.f2.li -text "Input jacks:"]
  foreach jack [audio inputs] {
    audio input $jack v(i$jack)
    pack [checkbutton .f.f2.b$jack -text $jack -variable v(i$jack)] -anc w
  }
}
if {[llength [audio outputs]] > 0} {
  pack [label .f.f2.lo -text "Output jacks:"]
  foreach jack [audio outputs] {
    audio output $jack v(o$jack)
    pack [checkbutton .f.f2.b$jack -text $jack -variable v(o$jack)] -anc w
  }
}
