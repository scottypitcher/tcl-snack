#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

package require -exact snack 1.7

option add *font {Helvetica 10 bold}

set file [lindex $argv 0]
snack::sound s -file _record[pid].wav -debug 0
if [file exists $file] {
    s configure -file $file
}
wm title . "record: $file"

set feedback Waveform
set sampfreq [s cget -frequency]
set sampfmt  [s cget -format]
set chan     [s cget -channels]
set width 600
set time "Length 00:00.00"
set secs [expr int([s length -units seconds])]
set dec [format "%.2d" [expr int(100*([s length -units seconds] - $secs))]]
set time [clock format $secs -format "Length %M:%S.$dec"]
set afterid ""
set max ([s max]
set min [s min])

if {$::tcl_platform(platform) == "unix"} {
    bind all <Control-c> Exit
}
wm protocol . WM_DELETE_WINDOW Exit

snack::menuInit
snack::menuPane File
snack::menuCommand File Open... OpenFile
snack::menuCommand File {Save As...} SaveFile
snack::menuCommand File Exit Exit

snack::menuPane Samplefreq
foreach f [snack::audio frequencies] {
    snack::menuRadio Samplefreq $f sampfreq $f "s configure -frequency $f"
}

snack::menuPane Format 5
foreach f [snack::audio formats] {
    snack::menuRadio Format $f sampfmt $f "s config -format $f"
}

snack::menuPane Channel
snack::menuRadio Channel Mono chan 1 "s config -channels Mono"
snack::menuRadio Channel Stereo chan 2 "s config -channels Stereo"

if {[snack::mixer inputs] != ""} {
    snack::menuPane Input
    foreach jack [snack::mixer inputs] {
	snack::mixer input $jack v(in$jack)
	snack::menuCheck Input $jack v(in$jack)
    }
}

if {[snack::mixer outputs] != ""} {
    snack::menuPane Output
    foreach jack [snack::mixer outputs] {
	snack::mixer output $jack v(out$jack)
	snack::menuCheck Output $jack v(out$jack)
    }
}

snack::menuPane Gain
snack::menuCommand Gain {Gain Control} {snack::gainBox pr}

snack::menuPane Visual
snack::menuRadio Visual Waveform feedback Waveform Draw
snack::menuRadio Visual Spectrogram feedback Spectrogram Draw
snack::menuRadio Visual Nothing feedback Nothing Draw

snack::createIcons
pack [ frame .f] -anchor w
pack [ button .f.bO -image snackOpen -command OpenFile] -side left
pack [ button .f.b3 -image snackSave -command SaveFile] -side left
pack [frame .f.f1 -width 1 -height 20 -highlightth 1] -side left -padx 5
pack [ button .f.bR -bitmap snackRecord -fg red -command Record] -side left
pack [ button .f.bS  -bitmap snackStop -command {s stop ; after cancel $afterid}] -side left
pack [ button .f.b2 -bitmap snackPlay -command {s play}] -side left

pack [ frame .f2] -anchor w
pack [ label .f2.l   -textvar time] -side left
pack [ label .f2.max -textvar max -wi 6]  -side left
pack [ label .f2.min -textvar min -wi 6]  -side left

proc OpenFile {} {
    global s file sampfreq sampfmt time max min

    set file [file tail $file]
    set file [snack::getOpenFile -initialfile $file]
    if [string compare $file ""] {
	s configure -file $file
	set sampfreq [s cget -frequency]
	set sampfmt  [s cget -format]
	set secs [expr int([s length -units seconds])]
	set dec [format "%.2d" [expr int(100*([s length -units seconds] - $secs))]]
	set time [clock format $secs -format "Length %M:%S.$dec"]
	set max ([s max]
	set min [s min])
    }
    wm title . "record: $file"
}

proc SaveFile {} {
    global s file

    set file [file tail $file]
    set file [snack::getSaveFile -initialfile $file]
    if [string compare $file ""] {
	if [string match [s cget -file] _record[pid].wav] {
	    file rename -force _record[pid].wav $file
	} else {
	    file copy -force [s cget -file] $file
	}
	s configure -file $file
    }
}

proc Record {} {
    global afterid
    catch {file delete -force _record[pid].wav}
    s configure -file _record[pid].wav
    s record
    set afterid [after 100 Update]
}

proc Draw {} {
    global feedback width file

    catch {destroy .c}
    if {$feedback == "Waveform"} {
	pack [ canvas .c -width $width -height 50] -before .f2
	snack::deleteInvalidShapeFile [file tail $file]
	.c create waveform 0 0 -sound s -height 50 -width $width -tags w \
  	       -pixels 250 -shapefile [file rootname [file tail $file]].shape \
		-progress snack::progressCallback 
	snack::makeShapeFileDeleteable [file tail $file]
    } elseif {$feedback == "Spectrogram"} {
	pack [ canvas .c -width $width -height 50] -before .f2
	.c create spectrogram 0 0 -sound s -height 50 -width $width -tags w \
		-pixels 250
    }
}

proc Update {} {
    global time afterid feedback max min

    set secs [expr int([s length -units seconds])]
    set dec [format "%.2d" [expr int(100*([s length -units seconds] - $secs))]]
    set time [clock format $secs -format "Length %M:%S.$dec"]
    set afterid [after 100 Update]
    set max ([s max]
    set min [s min])
}

proc Exit {} {
    catch {file delete -force _record[pid].wav}
    exit
}

Draw
