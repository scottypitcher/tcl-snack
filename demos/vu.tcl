#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

package require -exact snack 1.7

snack::sound s -channels 2

pack [canvas .c -width 400 -height 100]

foreach {top bot tag} {20 40 l 60 80 r} {
  .c create rect   0 $top 170 $bot -fill green  -outline ""
  .c create rect 170 $top 240 $bot -fill yellow -outline ""
  .c create rect 240 $top 327 $bot -fill red    -outline ""
  .c create rect 0 $top 327 $bot -fill black -stip gray50 -outli "" -tag $tag
}

pack [frame .f]
pack [button .f.a -text On -command On] -side left
pack [button .f.b -text Off -command {s stop}] -side left

proc On {} {
  s record
  after 100 Draw
}

proc Draw {} {
  set l [s max -start 0 -end -1 -channel 0]
  set r [s max -start 0 -end -1 -channel 1]
  s length 0

  .c coords l [expr $l/100] 20 327 40
  .c coords r [expr $r/100] 60 327 80
  if ![snack::audio active] return
  after 100 Draw
}
