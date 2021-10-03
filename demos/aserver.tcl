#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6

sound snd -debug 0
set port 23654

proc Cmd { sock addr port } {
    global servsock msg

    snd stop
    switch [read $sock 4] {
	play {
	    snd config -channel $sock -guessproperties yes
	    snd play -command "[list close $sock]; set msg idle"
	    set msg playing
	}
	stop {
	    close $sock
	    set msg idle
	}
	exit {
	    close $sock
	    close $servsock
	    exit
	}
    }
}

set servsock [socket -server Cmd $port]

# Make sure the server socket always is closed properly on exit

wm protocol . WM_DELETE_WINDOW {close $servsock; exit}

proc NewPort {} {
    global servsock port
    close $servsock
    set servsock [socket -server Cmd $port]
}

set msg idle
pack [frame .top]
pack [label .top.l1 -text Status:] -side left
pack [label .top.l2 -textvar msg -width 7] -side left

pack [frame .mid]
pack [label .mid.l -text Port] -side left
pack [entry .mid.e -textvar port -width 6] -side left
pack [button .mid.b -text Set -command NewPort] -side left

set gain [audio play_gain]
pack [frame .bot]
pack [button .bot.bs -bitmap stop -com {snd stop; set msg idle}] -side left
pack [button .bot.bp -bitmap pause -com {snd pause}] -side left
pack [scale .bot.s -show no -orient horiz -com {audio play_gain} -var gain]\
	-side left

vwait forever
