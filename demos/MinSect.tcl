#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

pack [canvas .c -width 300 -height 300]

sound s -load ex1.wav

.c create section 0 0 -sound s -start 6000 -end 6100 -height 300 -width 300

