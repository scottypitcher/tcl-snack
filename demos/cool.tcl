#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

# An example how to build a sound application using Snack.
# Can also be used as a base for specialized applications.

package require -exact snack 1.7

snack::sound snd -debug 0
set v(freq) 16000
set v(width) 600
set v(height) 150
set v(pps) 10
set v(start) 0
set v(end) [snd length]
set v(pausex) -1
set v(x0) 0
set v(fileName) ""

wm protocol . WM_DELETE_WINDOW exit

pack [set s [scrollbar .scroll -orient horiz -command Scroll]] -fill x
$s set 0 1
#bind $s <ButtonRelease-1> Redisplay

pack [set c [canvas .c -width $v(width) -height $v(height) -highlightthi 0]] -expand yes -fill both
$c create waveform 0 0 -sound snd -height $v(height) -width $v(width) -tag [list obj wave] -progress snack::progressCallback -debug 0
if [string match macintosh $::tcl_platform(platform)] {
    $c create rect  -1 -1 -1 -1 -tags mark -width 2 -outline red
} else {
    $c create rect  -1 -1 -1 -1 -tags mark -fill yellow -stipple gray25 \
	    -width 2 -outline red
}
$c create line -1 -1 -1 -1 -fill red -tags playmark

bind $c <ButtonPress-1>   { Button1Press %x }
bind $c <ButtonRelease-1> { Button1Release }
bind $c <Configure> Reconfigured
bind $c <Double-Button-1> ClearMark

pack [frame .f] -side bottom -before $c -fill x
pack [button .f.pl -bitmap snackPlay -command {Play 0}] -side left
pack [button .f.pa -bitmap snackPause -command Pause] -side left
pack [button .f.st -bitmap snackStop -command Stop] -side left
snack::createIcons
pack [button .f.op -image snackOpen -command LoadSound] -side left
pack [button .f.zi -image snackZoomIn -command ZoomIn] -side left
pack [button .f.zo -image snackZoomOut -command ZoomOut] -side left
pack [radiobutton .f.rs -text Spectrogram -com DrawSpectrogram -val 1] -side left
pack [radiobutton .f.rw -text Waveform -com DrawWaveform -val ""] -side left
pack [label .f.l -textvar v(time)] -side left

proc ZoomIn {} {
    global v c s

    set co [$c coords mark]
    set start [expr int($v(start) + double($v(freq)) * [lindex $co 0] / $v(pps))]
    set end   [expr int($v(start) + double($v(freq)) * [lindex $co 2] / $v(pps))]
    if {$start == $end || [snd length] == 0} return

# Update scrollbar
    $s set [expr double($start)/[snd length]] [expr double($end)/[snd length]]

    set v(pps) [expr $v(width) / (double($end - $start) / $v(freq))]
    set v(start) $start
    set v(end)   $end
    ClearMark
    Redisplay
}

proc ZoomOut {} {
    global v c s

    set n 2.0
    set delta [expr int($v(freq) * $v(width) / $v(pps))]
    set start [expr int($v(start)-($n-1)/2*$delta)]
    set end   [expr int($v(start)+$delta+($n-1)/2*$delta)]
    if {$start < 0}        { set start 0 }
    if {$end > [snd length]} { set end [snd length] }
    if {$start == $end} return

# Update scrollbar
    $s set [expr double($start)/[snd length]] [expr double($end)/[snd length]]

    set v(pps) [expr $v(width) / (double($end - $start) / $v(freq))]
    set v(start) $start
    set v(end)   $end
    ClearMark
    Redisplay
}

proc Scroll args {
    global v s

    set delta [expr int($v(freq) * $v(width) / $v(pps))]
    if {[lindex $args 0] == "moveto"} {
	set v(start) [expr int([snd length] * [lindex $args 1])]
    } elseif {[lindex $args 0] == "scroll"} {
	if {[lindex $args 1] > 0} {
	    set v(start) [expr $v(start)+$delta]
	} else {
	    set v(start) [expr $v(start)-$delta]
	}
    }
    if {$v(start) < 0} { set v(start) 0 }
    if {[expr $v(start)+$delta] > [snd length]} {
	set v(start) [expr [snd length]-$delta]
    }
    set v(end) [expr $v(start)+$delta]

# Update scrollbar
    $s set [expr double($v(start))/[snd length]] [expr double($v(end))/[snd length]]
    ClearMark
    Redisplay
}

proc Redisplay {} {
    global v c

# Display section [$start, $end] of the sound
    $c itemconf obj -start $v(start) -end $v(end)
}

proc Button1Press {x} {
    global c

    set xc [$c canvasx $x]
    $c raise mark
    $c coords mark $xc 0 $xc [expr [winfo height $c]-2]
    bind $c <Motion> { Button1Motion %x }
}

proc Button1Motion {x} {
    global c

    set xc [$c canvasx $x]
    if {$xc < 0} { set xc 0 }
    if {$xc > [winfo width $c]} { set xc [winfo width $c] }
    set co [$c coords mark]
    $c coords mark [lindex $co 0] 0 $xc [expr [winfo height $c]-2]
    ShowTime
}

proc Button1Release {} {
    global c

    bind $c <Motion> {}
    ShowTime
}

proc DrawSpectrogram {} {
    global v c

    $c delete obj
    set colors {#000 #006 #00B #00F #03F #07F #0BF #0FF #0FB #0F7 \
	    #0F0 #3F0 #7F0 #BF0 #FF0 #FB0 #F70 #F30 #F00}
    $c create spectrogram 0 0 -sound snd -height [winfo height $c]  \
	    -width [winfo width $c] -start $v(start) -end $v(end) \
	    -colormap $colors -tag obj -debug 0
    $c lower obj
}

proc DrawWaveform {} {
    global v c

    $c delete obj
    if {$v(fileName) == ""} {
	$c create waveform 0 0 -sound snd -height [winfo height $c] -debug 0 \
		-width [winfo width $c] -tag [list obj wave]
    } else {
	snack::deleteInvalidShapeFile [file tail $v(fileName)]
	$c create waveform 0 0 -sound snd -height [winfo height $c] -debug 0 \
		-width [winfo width $c] -start $v(start) -end $v(end) \
		-tag [list obj wave] -progress snack::progressCallback
	snack::makeShapeFileDeleteable [file tail $v(fileName)]
    }
    $c lower obj
}

proc LoadSound {} {
    global v c s

    set fileName [snack::getOpenFile]
    if {$fileName == ""} return
    $c itemconf wave -sound ""
    snd config -file $fileName -guessproperties yes
    set v(freq) [snd cget -frequency]
    set v(start) 0
    set v(end) [snd length]
    set v(pps) [expr $v(width) / (double($v(end) - $v(start)) / $v(freq))]
    set v(fileName) $fileName
# Update scrollbar
    $s set 0.0 1.0
    wm title . [file tail $fileName]
    snack::deleteInvalidShapeFile [file tail $fileName]
    $c itemconf wave -sound snd -start $v(start) -end $v(end) \
	    -shapefile [file rootname [file tail $fileName]].shape
    snack::makeShapeFileDeleteable [file tail $fileName]
    Redisplay
    ShowTime
}

proc ClearMark {} {
    global c

    $c coords mark -1 -1 -1 -1
    ShowTime
}

proc Reconfigured {} {
    global v c

    if {$v(end) == $v(start)} return
    set co [$c coords mark]
    if {[lindex $co 0] != -1} {
	set start [expr int($v(start) + double($v(freq))*[lindex $co 0] / $v(pps))]
	set end   [expr int($v(start) + double($v(freq))*[lindex $co 2] / $v(pps))]
	set x0temp [expr int($v(start) + double($v(freq))*$v(x0) / $v(pps))]
    }
    set newHeight [winfo height $c]
    set newWidth  [winfo width $c]
    $c itemconf obj -height $newHeight -width $newWidth
    set v(pps) [expr $newWidth / (double($v(end) - $v(start)) / $v(freq))]
    set v(width)  $newWidth
    set v(height) $newHeight
    if {[lindex $co 0] != -1} {
	set left  [expr double($start - $v(start))/$v(freq)*$v(pps)]
	set right [expr double($end   - $v(start))/$v(freq)*$v(pps)]
	set v(x0) [expr double($x0temp - $v(start))/$v(freq)*$v(pps)]
	$c coords mark $left 0 $right [expr [winfo height $c]-2]
    }
}

proc Play x {
    global v c s

    snd stop
    set c0 [lindex [$c coords mark] 0]
    set c2 [lindex [$c coords mark] 2]
    if {$x == 0} {
	set x $c0
	if {$c0 == -1} {
	    set l $v(start)
	    set r $v(end)	    
	} elseif {$c0 == $c2} {
	    set l [expr int($v(start) + double($v(freq)) * $c0 / $v(pps))]
	    set r $v(end)
	} else {
	    set l [expr int($v(start) + double($v(freq)) * $c0 / $v(pps))]
	    set r [expr int($v(start) + double($v(freq)) * $c2 / $v(pps))]
	}
    } else {
	if {$c0 == $c2} {
	    set l [expr int($v(start) + double($v(freq)) * $x / $v(pps))]
	    set r $v(end)
	} else {
	  set l [expr int($v(start) + double($v(freq)) * $x / $v(pps))]
	  set r [expr int($v(start) + double($v(freq)) * $c2 / $v(pps))]
	}
    }
    snd play -start $l -end $r
    after 0 PutPlayMarker $x
}

proc Pause {} {
    global v

    if [snack::audio active] {
	set v(pausex) [expr $v(x0) + $v(pps) * [snack::audio elapsedTime]]
	snd stop
    } elseif {$v(pausex) != -1} {
	Play $v(pausex)
    }
}

proc Stop {} {
    global v

    snd stop
    set v(pausex) -1
}

proc PutPlayMarker args {
    global v c

    if ![snack::audio active] {
	$c coords playmark -1 -1 -1 -1
	ShowTime
	return
    }
    if {$args != ""} {
	set v(x0) [lindex $args 0]
    }
    set x [expr $v(x0) + $v(pps) * [snack::audio elapsedTime]]
    set co [$c coords mark]
    if {[lindex $co 0] != [lindex $co 2] && $x > [lindex $co 2]} {
	$c coords playmark -1 -1 -1 -1
	ShowTime
	return
    }
    $c coords playmark $x 0 $x $v(height)
    after 50 PutPlayMarker
    set time [expr int($v(start) + double($v(freq)) * $x / $v(pps))]
    set v(time) "Time: [SampleIndex2Time $time]"
}

proc ShowTime {} {
    global v c

    set co [$c coords mark]
    set start [expr int($v(start) + double($v(freq)) * [lindex $co 0] / $v(pps))]
    set end   [expr int($v(start) + double($v(freq)) * [lindex $co 2] / $v(pps))]
    if {[lindex $co 0] == -1} {
	set v(time) "Length: [SampleIndex2Time [snd length -units samples]]"
	return
    }
    set v(t1) [SampleIndex2Time $start]
    set v(t2) [SampleIndex2Time $end]
    if {$end == $start} {
	set v(time) "Time: $v(t1)"
	return
    }
    set v(time) "\[$v(t1)-$v(t2)\]"
}

proc SampleIndex2Time index {
    global v

    set sec [expr int($index / $v(freq))]
    set dec [format "%.2d" [expr int(100*((double($index) / $v(freq))-$sec))]]
    return [clock format $sec -format "%M:%S.$dec"]
}
