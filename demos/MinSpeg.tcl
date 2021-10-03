#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

pack [canvas .c]

sound s -load ex1.wav

.c create spectrogram 0 0 -sound s -height 260 -width 400

