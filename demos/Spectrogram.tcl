#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

set width 300
set height 200
set pps 300
set bright 0.0
set contrast 0.0
set winlen 128
set fftlen 256
set gridfspacing 0
set gridtspacing 0.0
set filename spectrogram.ps
set colors {#000 #006 #00B #00F #03F #07F #0BF #0FF #0FB #0F7 \
	    #0F0 #3F0 #7F0 #BF0 #FF0 #FB0 #F70 #F30 #F00}
set color Red
option add *font {Helvetica 10 bold}

pack [ canvas .c -width 600 -height 300]
pack [ label .l -text "Drag spectrogram with left mouse button"]
pack [ frame .f1] -pady 2
pack [ scale .f1.s1 -variable width -label Width -from 10 -to 600 -orient hori\
	-length 100 -command {.c itemconf speg -width }] -side left
pack [ scale .f1.s2 -variable height -label Height -from 10 -to 300 -orient\
	hori -length 100 -command {.c itemconf speg -height }] -side left
pack [ scale .f1.s3 -variable pps -label Pix/sec -from 10 -to 600 -orient hori\
	-length 100 -command {.c itemconf speg -pixelspersec }] -side left
pack [ scale .f1.s4 -variable bright -label Brightness -from -100 -to 100\
	-res 0.1 -orient hori -length 100 -command {.c itemconf speg -brightness }] -side left
pack [ scale .f1.s5 -variable contrast -label Contrast -from -100 -to 100 -res 0.1 -orient hori -length 100 -command {.c itemconf speg -contrast }] -side left

set topfr 8000
pack [ scale .f1.s7 -variable topfr -label Top -from 1000 -to 8000 -orient hori -length 100 -command {.c itemconf speg -topfr }] -side left

pack [ frame .f2] -pady 2
pack [ label .f2.lw -text "Hamming window:"] -side left
foreach n {32 64 128 256 512 1024 2048} {
    pack [ radiobutton .f2.w$n -text $n -variable winlen -value $n\
	    -command {.c itemconf speg -win $winlen}] -side left
}

pack [ frame .f3] -pady 2
pack [ label .f3.lf -text "FFT points:"] -side left
foreach n {64 128 256 512 1024 2048 4096} {
    pack [ radiobutton .f3.f$n -text $n -variable fftlen -value $n\
	    -command {.c itemconf speg -fft $fftlen}] -side left
}

pack [ frame .f4] -pady 2
pack [ label .f4.lf -text "Grid f-spacing:"] -side left
foreach n {0 500 1000 2000} {
    pack [ radiobutton .f4.f$n -text $n -variable gridfspacing -value $n\
	    -command {.c itemconf speg -gridfspacing $gridfspacing}] -side left
}
pack [ label .f4.lf2 -text "Grid t-spacing:"] -side left
foreach n {0 1 25 5} {
    pack [ radiobutton .f4.t$n -text 0.$n -variable gridtspacing -value 0.$n\
	    -command {.c itemconf speg -gridtspacing $gridtspacing}] -side left
}

pack [ frame .f42] -pady 2
pack [ label .f42.lf3 -text "Grid color:"] -side left
foreach f {Black Red Blue White Cyan} {
    pack [ radiobutton .f42.c$f -text $f -variable color -value $f \
	    -command {.c itemconf speg -gridcolor $color}] -side left
}

pack [ frame .f5] -pady 2
pack [ button .f5.br -bitmap record -command Record -fg red] -side left
pack [ button .f5.bs -bitmap stop -command {s stop}] -side left
pack [ label .f5.l -text "Load sound file:"] -side left
pack [ button .f5.b1 -text ex1.wav -command {s read ex1.wav}] -side left
pack [ button .f5.b2 -text ex2.wav -command {s read ex2.wav}] -side left

proc Record {} {
    global width pps
    
    s flush
    .c itemconf speg -pixelspersecond $pps -width $width
    s record
    after 10000 {.f5.bs invoke}
}

set col ""
pack [ frame .f6] -pady 2
pack [ label .f6.l1 -text "Colors:"] -side left
pack [ radiobutton .f6.r1 -text B/W -var col -val "" -command {.c itemconf speg -colormap $col}] -side left
pack [ radiobutton .f6.r2 -text Rainbow -var col -val $colors -command {.c itemconf speg -colormap $col}] -side left
pack [ label .f6.l2 -text "Generate postscript file:"] -side left
pack [ entry .f6.e -text filename] -side left
pack [ button .f6.b -text Save -command {.c postscript -file $filename}] -side left

pack [ button .bExit -text Exit -command exit]

sound s -load ex1.wav

update

.c create spectrogram 300 150 -anchor c -sound s -height $height -width $width -tags speg -pixelsp $pps

source widutil.tcl
BindDrag .c
