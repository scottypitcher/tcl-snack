#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

package require -exact snack 1.7

pack [canvas .c -width 400 -height 100]

snack::sound s -load ex1.wav

.c create waveform 0 0 -sound s -width 400

pack [button .bExit -text Exit -command exit]
