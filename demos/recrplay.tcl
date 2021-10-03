#!/usr/local/bin/wish8.2

package require -exact snack 1.6

sound s

set last 0
set server localhost:23654

proc Start {} {
    global sock server

    s record

    # Open binary socket connection to aserver.tcl

    foreach {host port} [split $server :] break
    set sock [socket $host $port]
    fconfigure $sock -translation binary
    if {$::tcl_version > 8.0} {
	fconfigure $sock -encoding binary
    }

    # Notify audio server that a play operation is due

    puts -nonewline $sock play

    # Send an AU file header to open the device correctly

    puts -nonewline $sock [s data -fileformat au]

    # Run this procedure again in 200ms

    after 200 SendData
}

proc Stop {} {
    s stop
}

proc SendData {} {
    global last sock

    # There is new sound data to send

    if {[s length] > $last} {

	# Send audio data chunk in AU file format, "bigEndian"

	puts -nonewline $sock [s data -start $last -end -1 -fileformat raw\
		-byteorder bigEndian]
    }
    set last [s length]
    .l config -text Length:[s length]

    # User hit stop button, close down

    if ![audio active] {
	set last 0
	close $sock
	return
    }
    after 300 SendData
}

pack [label .l -text "Length: 0"]

pack [frame .f1]
pack [label .f1.l -text "Server:"] -side left
pack [entry .f1.e -textvar server] -side left

pack [frame .f2]
pack [button .f2.a -bitmap record -com Start -wi 40 -he 20 -fg red] -si left
pack [button .f2.b -bitmap stop -com Stop -wi 40 -he 20] -side left
pack [button .f2.c -bitmap play -com {s play} -wi 40 -he 20] -side left
pack [button .f2.d -text Exit -command exit] -side left
