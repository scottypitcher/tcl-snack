#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

set width 300
set height 200
set start 0
set end 48000
set stipple ""
set winlen 256
set fftlen 512
set filename section.ps
set topfr 8000
set maxval 0.0
set minval -80.0
option add *font {Helvetica 10 bold}

pack [ canvas .c -width 400 -height 250]
pack [set c [canvas .c2 -height 50 -width 400 -closeenough 5]]
pack [ label .l -text "Drag markers with left mouse button"]
pack [ frame .f1] -pady 2
pack [ scale .f1.s1 -variable width -label Width -from 10 -to 400 \
	-orient hori -length 100 -command {.c itemconf sect -width }] -side left
pack [ scale .f1.s2 -variable height -label Height -from 10 -to 250 \
	-orient hori -length 100 -command {.c itemconf sect -height }] -side left
pack [ scale .f1.s3 -variable topfr -label "Top frequency" -from 1000 -to 8000 -orient hori -length 100 -command {.c itemconf sect -topfr }] -side left
pack [ scale .f1.s4 -variable maxval -label "Max value" -from 40 -to -40 -orient hori -length 100 -command {.c itemconf sect -maxvalue }] -side left
pack [ scale .f1.s5 -variable minval -label "Min value" -from -20 -to -100 -orient hori -length 100 -command {.c itemconf sect -minvalue }] -side left

pack [ frame .f2i] -pady 2
pack [ label .f2i.lw -text "Hamming window:"] -side left
foreach n {32 64 128 256 512} {
    pack [ radiobutton .f2i.w$n -text $n -variable winlen -value $n -command {.c itemconf sect -win $winlen}] -side left
}

pack [ frame .f3i] -pady 2
pack [ label .f3i.lf -text "FFT points:"] -side left
foreach n {64 128 256 512 1024} {
    pack [ radiobutton .f3i.f$n -text $n -variable fftlen -value $n -command {.c itemconf sect -fft $fftlen}] -side left
}

pack [ frame .f2] -pady 2
pack [ checkbutton .f2.f -text Frame -variable frame \
	-command {.c itemconf sect -frame $frame}] -side left
set frame 1

foreach color {Black Red Blue} {
pack [ radiobutton .f2.c$color -text $color -variable color -value $color \
	-command {.c itemconf sect -fill $color}] -side left
}
set color Black

pack [ radiobutton .f2.s100 -text 100% -variable stipple -value "" \
	-command {.c itemconf sect -stipple $stipple}] -side left
pack [ radiobutton .f2.s50 -text 50% -variable stipple -value gray50 \
	-command {.c itemconf sect -stipple $stipple}] -side left
pack [ radiobutton .f2.s25 -text 25% -variable stipple -value gray25 \
	-command {.c itemconf sect -stipple $stipple}] -side left

pack [ frame .f3] -pady 2
pack [ button .f3.br -bitmap record -command Record -fg red] -side left
pack [ button .f3.bs -bitmap stop -command {s stop}] -side left
pack [ label .f3.l -text "Load sound file:"] -side left
pack [ button .f3.b1 -text ex1.wav -command {s read ex1.wav}] -side left
pack [ button .f3.b2 -text ex2.wav -command {s read ex2.wav}] -side left

proc Record {} {
    s record
    after 10000 {.f3.bs invoke}
}

pack [ frame .f4] -pady 2
pack [ label .f4.l -text "Generate postscript file:"] -side left
pack [ entry .f4.e -text filename] -side left
pack [ button .f4.b -text Save -command {.c postscript -file $filename}] -side left

pack [ button .bExit -text Exit -command exit]

sound s -load ex1.wav

.c create section 200 125 -anchor c -sound s -height $height -width $width -tags sect -frame $frame -debug 0 -start 9002 -end 12000

$c create spectrogram 0 0 -sound s -height 50 -width 400 -tags s
$c create line        5 0 5 50     -tags m1
$c create line        395 0 395 50 -tags m2

$c bind m1 <B1-Motion> {
    $c coords m1 [$c canvasx %x] 0 [$c canvasx %x] 100
    .c itemconf sect -start [expr int(16000 * [$c canvasx %x] / 600)]
}
$c bind m2 <B1-Motion> {
    $c coords m2 [$c canvasx %x] 0 [$c canvasx %x] 100
    .c itemconf sect -end [expr int(16000 * [$c canvasx %x] / 600)]
}

source widutil.tcl
BindDrag .c
