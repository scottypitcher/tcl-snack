#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

pack [canvas .c -width 400 -height 100]

sound s -load ex1.wav

.c create waveform 0 0 -sound s -width 400

