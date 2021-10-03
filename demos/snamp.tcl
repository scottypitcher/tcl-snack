#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

sound s -debug 0
sound s2

set timestr ""
option add *font {Helvetica 10 bold}
wm title . "Snack Audio MPEG Player"

pack [frame .frame] -side top -expand yes -fill both
scrollbar .frame.scroll -command ".frame.list yview"
listbox .frame.list -yscroll ".frame.scroll set" -setgrid 1 -selectmode single -exportselection false -height 16
pack .frame.scroll -side right -fill y
pack .frame.list -side left -expand 1 -fill both
bind .frame.list <Double-ButtonPress-1> Play
bind .frame.list <B1-Motion> {Drag %y}
bind .frame.list <ButtonPress-1> {Select %y}
bind . <BackSpace> Cut

snack::createIcons
pack [frame .panel] -side bottom -before .frame
pack [button .panel.bp -bitmap play -com Play] -side left
pack [button .panel.bs -bitmap stop -com Stop] -side left
pack [button .panel.bo -image snackOpen -com Open] -side left
set p 0
pack [scale .panel.ss -show no -orient horiz -len 130 -var p] -side left
set gain [audio play_gain]
pack [scale .panel.sv -show no -orient horiz -com {audio play_gain} -len 70 -var gain] -side left
set setdrag 1
bind .panel.ss <ButtonPress-1> {set setdrag 0}
bind .panel.ss <ButtonRelease-1> {set setdrag 1 ; Play2}
pack [label .panel.l -textvar timestr]

proc Open {} {
    global files
    set file [snack::getOpenFile -format MP3]
    if {$file != ""} {
	set name [lindex [file split $file] end]
	set files($name) $file
	.frame.list insert end $name
    }
}

proc Play args {
    global files t0 filelen
    if {[.frame.list curselection] == ""} {
	set i 0
    } else {
	set i [.frame.list curselection]
    }
    .frame.list selection set $i
    Stop
    s config -file $files([.frame.list get $i])
    if {$args == ""} {
	s play -command Next
	set t0 [clock scan now]
    } else {
	s play -start $args -command Next
	set t0 [expr [clock scan now] - $args / [s cget -frequency]]
    }
    set filelen [s length]
    Timer
}

proc Play2 {} {
    global filelen p
    Play [expr int($p/100.0*[s length])]
}

proc Stop {} {
    s stop
    after cancel Timer
}

proc Timer {} {
    global t0 timestr setdrag
    set time [expr [clock scan now] - $t0]
    set timestr [clock format $time -format "%M:%S"]
    if $setdrag {
	.panel.ss set [expr int(100 * $time / [s length -units sec])]
    }
    after 100 Timer
}

proc Next {} {
    set i [.frame.list curselection]
    if {$i == ""} return
    .frame.list selection clear $i
    incr i
    .frame.list selection set $i
    .frame.list see $i
    after 10 Play
}

set cut ""
proc Cut {} {
    global cut
    if {[.frame.list curselection] != ""} {
	set cut [.frame.list get [.frame.list curselection]]
	.frame.list delete [.frame.list curselection]
    }
}

proc Select y {
    global old timestr files
    set old [.frame.list nearest $y]
    s2 config -file $files([.frame.list get $old])
    set timestr [clock format [expr int([s2 length -units sec])] -format "%M:%S"]
}

proc Drag y {
    global old
    set new [.frame.list nearest $y]
    if {$new == -1} return
    set tmp [.frame.list get $old]
    .frame.list delete $old
    .frame.list insert $new $tmp
    .frame.list selection set $new
    set old $new
}

if [info exists argv] {
 if [file isdirectory $argv] {
  catch {cd $argv}
 }
}

foreach file [lsort -dictionary [glob -nocomplain *.mp3 *.wav]] {
    set name [lindex [file split $file] end]
    set files($name) $file
    .frame.list insert end $file
}
