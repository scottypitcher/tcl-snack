#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

package require -exact snack 2.1

set rate 16000
snack::sound s -rate $rate

set w 300
set h 300
set s 100
# Length of FFT 
set n 1024
set type FFT

# Start recording, create polygon, and schedule a draw in 100 ms

proc Start {} {
  set ::pos 0
  .c delete polar
  .c create polygon -1 -1 -1 -1 -tags polar -fill green
  s record
  after 100 Draw
}

# Stop recording and updating the plot

proc Stop {} {
  s stop
  after cancel Draw
}

# Calculate spectrum and plot it

proc Draw {} {
  if {[s length] > $::n} {
    set ::pos [expr [s length] - $::n]
    set spec [s dBPowerSpectrum -start $::pos -fftlen $::n -winlen $::n \
	-analysistype $::type]
    set coords {}
    set f 0.0001
    foreach val $spec {
      set v [expr {6.282985 * log($f)/log(2.0)}]
      set a [expr {1.4*($val+$::s)}]
      set x [expr {$::w/2+$a*cos($v)}]
      set y [expr {$::h/2+$a*sin($v)}]
      lappend coords $x $y
      set f [expr {$f + 16000.0 / $::n}]
    }
    eval .c coords polar $coords
  }
  after 10 Draw
  if {[s length -unit sec] > 20} Stop
}

# Create simple GUI

pack [canvas .c -width $::w -height $::h -bg black]
pack [ button .b1 -bitmap snackRecord -command Start -fg red] -side left
pack [ button .b2 -bitmap snackStop   -command Stop] -side left
pack [ radiobutton .b3 -text FFT -variable type -value FFT] -side left
pack [ radiobutton .b4 -text LPC -variable type -value LPC] -side left
