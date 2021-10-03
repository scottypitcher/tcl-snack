#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

package require -exact snack 2.1

set f [snack::filter generator 440.0]
snack::sound s
#snack::audio playLatency 200

set v(freq) 440.0
set v(ampl) 20000

pack [frame .f]
pack [scale .f.s1 -label Frequency -from 4000 -to 50 -length 200\
        -variable v(freq) -command Config] -side left
pack [scale .f.s2 -label Amplitude -from 32767 -to 0 -length 200\
        -variable v(ampl) -command Config] -side left

tk_optionMenu .m v(type) sine rectangle triangle sawtooth noise
foreach i [list 0 1 2 3 4] {
  .m.menu entryconfigure $i -command Config
}
pack .m

snack::createIcons
pack [frame .fb]
pack [button .fb.a -bitmap snackPlay -command Play] -side left
pack [button .fb.b -bitmap snackStop -command "s stop"] -side left

proc Config {args} {
  global f v
  set shape 0.0
  set type $v(type)
  switch $type {
    sine {
      set shape 0.0
    }
    rectangle {
      set shape 0.5
    }
    triangle {
      set shape 0.5
    }
    sawtooth {
      set shape 0.0
      set type triangle
    }
  }
  $f configure $v(freq) $v(ampl) $shape $type -1
}

proc Play {} {
  global f
  s stop
  s play -filter $f
}
