#!/bin/sh
# the next line restarts using wish \
exec wish8.2 "$0" "$@"

package require -exact snack 1.6
catch {
    package require snackSphere
    set package_sphere 1
}
package require http

set debug 0
sound snd -debug $debug
sound cbs -debug $debug

set tcl_precision 7
set f(prog) [info script]
set f(labfile) ""
set f(sndfile) ""
set f(lpath)   ""
set f(header)  ""
set mexhome "/u/kare/tcl/mexd"
catch {source $mexhome/ipa_tmh.tcl}
set f(ipapath) $mexhome/ipa_xbm
set local 0
if $local {
    set v(labfmt) MIX
    set v(smpfmt) SMP
    set v(remote) 0
    set v(ashost) datan.speech.kth.se
    set v(asport) 23654
} else {
    set v(labfmt) HTK
    set v(smpfmt) WAV
    set v(remote) 0
    set v(ashost) host.domain.xxx.yyy
    set v(asport) 23654
}
set labels {}
set undo {}
set v(labchanged) 0
set v(smpchanged) 0
set v(width) 600
set v(toth) 286
set v(msg) "Press right mouse button for menu"
set v(timeh) 15
set v(yaxisw) 40
set v(labelh) 20
set v(psfilet) {tmp$N.ps}
set v(psfile)  ""
set v(vchan)   -1
#set v(offset) 0
#set v(zerolabs) 0
set v(startsmp) 0
set v(lastmoved) -1
set v(p_version) 1.6
set v(s_version) 1.6
set v(plugins) {}
set v(scroll) 1
set v(freq) 16000
set v(sfmt) Lin16
set v(chan) 1
set v(topfr) 8000
set v(rp_sock) ""
#set v(propflag) 0
set v(pause) 0
set v(recording) 1
set v(activerec) 0
set v(cmap) grey
set v(grey) " "
#set v(color1) {#000 #006 #00B #00F #03F #07F #0BF #0FF #0FB #0F7 \
	      #0F0 #3F0 #7F0 #BF0 #FF0 #FB0 #F70 #F30 #F00}
set v(color1) {#000 #004 #006 #00A #00F \
 	       #02F #04F #06F #08F #0AF #0CF #0FF #0FE \
	       #0FC #0FA #0F8 #0F6 #0F4 #0F2 #0F0 #2F0 \
	       #4F0 #6F0 #8F0 #AF0 #CF0 #FE0 #FC0 #FA0 \
	       #F80 #F60 #F40 #F20 #F00}
set v(color2) {#FFF #BBF #77F #33F #00F #07F #0BF #0FF #0FB #0F7 \
	      #0F0 #3F0 #7F0 #BF0 #FF0 #FB0 #F70 #F30 #F00}
set v(contrast) 0
set v(brightness) 0
set v(showspeg) 0
set v(remspegh) 200

set z(zoomwinh) 200
set z(zoomwinw) 600
set z(zoomwavh) 0
set z(zoomwavw) 0
set z(f) 1

set s(sectwinh) 400
set s(sectwinw) 400
set s(secth) 400
set s(sectw) 400
set s(rx) -1
set s(fftlen) 512
set s(winlen) 512
set s(anabw)  31.25

proc SetDefaultVars {} {
    global f v local

    set v(waveh) 50
    set v(spegh) 0
    set v(scrw) 32767
    set v(pps) 400
    set v(opps) 400
    set v(npt) 100
    set v(fftlen) 256
    set v(winlen) 128
    set v(anabw) 125
    set v(preemph) 0.97
    set v(ipa) 0
    set v(autoload) 0
    set v(ch) 0
    set v(iso) 1
    set v(slink) 0
    set v(mlink) 0
    if {$::tcl_platform(platform) == "unix"} {
	set v(printcmd)  {lpr $FILE}
	set v(gvcmd)     {ghostview $FILE}
	set v(psfilecmd) {cp -f _xspr$n.ps $v(psfile)}
	if $local {
	    set v(pluginfiles) {/u/kare/tcl/mexd/dataplot.plg /u/kare/tcl/mexd/generator.plg /u/kare/tcl/mexd/transcribe.plg /u/kare/tcl/mexd/cutter.plg /u/kare/tcl/mexd/pitch.plg}
	} else {
	    set v(pluginfiles) [glob -nocomplain *.plg]
	}
#	set v(browser) "netscape"
	if {$::tcl_platform(os) == "HP-UX"} {
	    option add *font {Helvetica 10 bold}
	} else {
	    option add *font {Helvetica 12 bold}
	}
    } else {
	set v(printcmd)  {C:/gstools/gs5.50/gswin32 "-IC:\gstools\gs5.50;C:\gstools\gs5.50\fonts" -sDEVICE=laserjet -dNOPAUSE $FILE -c quit}
	set v(gvcmd)     {C:/gstools/gsview/gsview32 $FILE}
	set v(psfilecmd) {command.com /c copy _xspr$n.ps $v(psfile)}
	if $local {
#	    set v(pluginfiles) {H:/tcl/mexd/dataplot.plg H:/tcl/mexd/generator.plg H:/tcl/mexd/pitch.plg}
            set v(pluginfiles) {}
	} else {
	    set v(pluginfiles) [glob -nocomplain *.plg]
	}
#	set v(browser) "c:/program files/netscape/communicator/program/netscape.exe"
    }
    set v(ipafmt) TMH
    set v(labalign) w
    set v(fg) black
    set v(bg) [. cget -bg]
    set v(fillmark) 1
    set v(font)  {Courier 10}
    set v(sfont) {Helvetica 8 bold}
    set v(gridfspacing) 0
    set v(gridtspacing) 0
    set v(gridcolor) red
    set v(cmap) grey
    set v(showspeg) 0
    set v(remspegh) 200
    set v(linkfile) 0
    set f(skip) 0
#    set f(byteOrder) $::tcl_platform(byteOrder)
    set f(byteOrder) ""
    set f(ipath) ""
    set f(ihttp) "http://www.speech.kth.se/~kare/dogbells.mp3"
    set f(guessraw) 1
    #"http://www.speech.kth.se/cgi-bin/TransAll?this_is_an_example+am"
}

SetDefaultVars
catch { source ~/.xsrc }

snd config -frequency $v(freq)
snd config -format $v(sfmt)
snd config -channels $v(chan)

set f(spath) $f(ipath)
set f(http) $f(ihttp)
set f(urlToken) ""

if {$v(p_version) != $v(s_version)} {
     set v(msg) "Warning, you have saved settings from an older version of xs!"
    SetDefaultVars
}

# Put custom settings between the lines below
# Custom settings start here
# Custom settings end here

snack::menuInit
snack::menuPane File
snack::menuCommand File {Open...} GetOpenFileName
snack::menuBind . o File {Open...}
snack::menuCommand File {Get URL...} OpenGetURLWindow
snack::menuCommand File Save Save
snack::menuBind . s File Save
snack::menuCommand File {Save As...} SaveAs
snack::menuCommand File Close Close
snack::menuSeparator File
snack::menuCommand File Print... {Print .cf.fc.c -1}
snack::menuCommand File Info {set v(msg) [InfoStr nopath]}
snack::menuSeparator File
snack::menuCommand File Exit Exit

snack::menuPane Edit
snack::menuCommand Edit Undo Undo
snack::menuEntryOff Edit Undo
snack::menuSeparator Edit
snack::menuCommand Edit Cut Cut
snack::menuBind . x Edit Cut
snack::menuCommand Edit Copy Copy
snack::menuBind . c Edit Copy
snack::menuCommand Edit Paste Paste
snack::menuBind . v Edit Paste
snack::menuCommand Edit Crop Crop
snack::menuCommand Edit Reverse Reverse
snack::menuCommand Edit {Mark All} MarkAll
snack::menuCommand Edit {Zero Cross Adjust} ZeroXAdjust

set n [snack::menuPane Audio]
bind $n <<MenuSelect>> { audio update }
snack::menuCommand Audio {Play range} PlayMark
snack::menuCommand Audio {Play All} PlayAll
snack::menuBind . p Audio {Play All}
snack::menuCommand Audio {Stop Play} StopPlay
#snack::menuCommand Audio {Gain Control...} {snack::gainBox rp}
snack::menuCommand Audio Mixer... snack::mixerDialog
#if {[audio inputs] != ""} {
#    snack::menuCascade Audio Input
#    foreach jack [audio inputs] {
#	audio input $jack v(in$jack)
#	snack::menuCheck Input $jack v(in$jack)
#    }
#}
#if {[audio outputs] != ""} {
#    snack::menuCascade Audio Output
#    foreach jack [audio outputs] {
#	audio output $jack v(out$jack)
#	snack::menuCheck Output $jack v(out$jack)
#    }
#}
snack::menuCascade Audio {Audio settings}
snack::menuCascade {Audio settings} {Set frequency}
set freqList [audio frequencies]
if {$freqList == ""} {
    set freqList {11025 22050 44100}
}
foreach fr $freqList {
    snack::menuRadio {Set frequency} $fr v(freq) $fr SetRaw
}
snack::menuCascade {Audio settings} {Set format}
foreach fo [list Lin16 Mulaw Alaw Lin8offset Lin8] {
    snack::menuRadio {Set format} $fo v(sfmt) $fo SetRaw
}
snack::menuCascade {Audio settings} {Set channels}
snack::menuRadio {Set channels} Mono   v(chan) 1 SetRaw
snack::menuRadio {Set channels} Stereo v(chan) 2 SetRaw
snack::menuCommand {Audio settings} {Swap bytes} {StopPlay;snd byteswap;Redraw}
snack::menuCascade Audio Conversions
snack::menuCascade Conversions {Convert frequency}
foreach fr $freqList {
    snack::menuCommand {Convert frequency} $fr "Convert {} $fr {}"
}
snack::menuCascade Conversions {Convert format}
foreach fo [list Lin16 Mulaw Alaw Lin8offset Lin8] {
    snack::menuCommand {Convert format} $fo "Convert $fo {} {}"
}
snack::menuCascade Conversions {Convert channels}
snack::menuCommand {Convert channels} Mono   "Convert {} {} Mono"
snack::menuCommand {Convert channels} Stereo "Convert {} {} Stereo"

snack::menuPane Tools

snack::menuPane Options
snack::menuCommand Options Settings... Settings
snack::menuCommand Options Plug-ins... Plugins
snack::menuCascade Options {Label File Format}
snack::menuRadio {Label File Format} TIMIT v(labfmt) TIMIT {Redraw quick}
snack::menuRadio {Label File Format} HTK v(labfmt) HTK {Redraw quick}
snack::menuRadio {Label File Format} WAVES v(labfmt) WAVES {Redraw quick}
snack::menuRadio {Label File Format} MIX v(labfmt) MIX {Redraw quick}
if $local {
    snack::menuCascade Options {IPA Translation}
    snack::menuRadio {IPA Translation} TMH v(ipafmt) TMH {source $mexhome/ipa_tmh.tcl;Redraw quick}
    snack::menuRadio {IPA Translation} CMU v(ipafmt) CMU {source $mexhome/ipa_cmu.tcl;Redraw quick}
}
snack::menuCascade Options {Label Alignment}
snack::menuRadio {Label Alignment} left v(labalign)   w {Redraw quick}
snack::menuRadio {Label Alignment} center v(labalign) c {Redraw quick}
snack::menuRadio {Label Alignment} right v(labalign)  e {Redraw quick}
snack::menuCascade Options {View Channel}
snack::menuRadio {View Channel} both v(vchan) -1 { Redraw;DrawZoom 1;DrawSect }
snack::menuRadio {View Channel} left v(vchan) 0  { Redraw;DrawZoom 1;DrawSect }
snack::menuRadio {View Channel} right v(vchan) 1 { Redraw;DrawZoom 1;DrawSect }
snack::menuSeparator Options
if $local {
    snack::menuCheck Options {IPA Transcription} v(ipa) {Redraw quick}
}
snack::menuCheck Options {Record Button} v(recording) ToggleRecording
snack::menuCheck Options {Show Spectrogram} v(showspeg) ToggleSpeg
snack::menuCheck Options {Auto Load} v(autoload)
snack::menuCheck Options {Guess Raw Format} f(guessraw)
snack::menuCheck Options {Cross Hairs} v(ch) DrawCrossHairs
snack::menuCheck Options {Fill Between Marks} v(fillmark) {$c coords mfill -1 -1 -1 -1 ; Redraw quick}
snack::menuCheck Options {Link to Disk File} v(linkfile) Link2File
if $local {
    snack::menuCheck Options {Show ÅÄÖ in labels} v(iso)
}
if {$tcl_platform(platform) == "unix"} {
    snack::menuCheck Options {Link Scroll} v(slink)
    snack::menuCheck Options {Link Marks} v(mlink)
}
#snack::menuCheck Options {Align x-axis/first label} v(offset) {Redraw quick}
#snack::menuCheck Options {Show zero length labels} v(zerolabs) {Redraw quick}
snack::menuSeparator Options
snack::menuCommand Options {Set default options} {SetDefaultVars ; Redraw}
snack::menuCommand Options {Save options} SaveSettings

snack::menuPane Window
snack::menuCommand Window {New Window} NewWin
snack::menuBind . n Window {New Window}
snack::menuCommand Window Refresh Redraw
snack::menuBind . r Window Refresh
snack::menuCommand Window {Waveform Zoom} OpenZoomWindow
snack::menuCommand Window {Spectrum Section} OpenSectWindow

snack::menuPane Help
snack::menuCommand Help Version Version
snack::menuCommand Help Manual  {Help http://www.speech.kth.se/snack/xs.html}

# Put custom menus between the lines below
# Custom menus start here
# Custom menus end here

#bind Menu <<MenuSelect>> {
#    global v
#    if {[catch {%W entrycget active -label} label]} {
#	set label ""
#    }
#    set v(msg) $label
#    update idletasks
#}

if {$tcl_platform(platform) == "windows"} {
    set border 1
} else {
    set border 0
}

snack::createIcons
pack [frame .tb -highlightthickness 1] -anchor w
pack [button .tb.open -command GetOpenFileName -image snackOpen -highlightthickness 0 -border $border] -side left

pack [button .tb.save -command Save -image snackSave -highlightthickness 0 -border $border] -side left
pack [button .tb.print -command {Print .cf.fc.c -1} -image snackPrint -highlightthickness 0 -border $border] -side left

pack [frame .tb.f1 -width 1 -height 20 -highlightth 1] -side left -padx 5
pack [button .tb.cut -command Cut -image snackCut -highlightthickness 0 -border $border] -side left
pack [button .tb.copy -command Copy -image snackCopy -highlightthickness 0 -border $border] -side left
pack [button .tb.paste -command Paste -image snackPaste -highlightthickness 0 -border $border] -side left

pack [frame .tb.f2 -width 1 -height 20 -highlightth 1] -side left -padx 5
pack [button .tb.undo -command Undo -image snackUndo -highlightthickness 0 -border $border -state disabled] -side left

pack [frame .tb.f3 -width 1 -height 20 -highlightth 1] -side left -padx 5
pack [button .tb.play -command PlayMark -bitmap play -fg blue3 -highlightthickness 0 -border $border] -side left
bind .tb.play <Enter> {SetMsg "Play mark"}
pack [button .tb.pause -command PausePlay -bitmap pause -fg blue3 -highlightthickness 0 -border $border] -side left
bind .tb.pause <Enter> {SetMsg "Pause"}
pack [button .tb.stop -command StopPlay -bitmap stop -fg blue3 -highlightthickness 0 -border $border] -side left
bind .tb.stop <Enter> {SetMsg "Stop"}
pack [button .tb.rec -command Record -bitmap record -fg red -highlightthickness 0 -border $border] -side left
bind .tb.rec <Enter> {SetMsg "Record"}
#pack [button .tb.gain -command {snack::gainBox rp} -image snackGain -highlightthickness 0 -border $border] -side left
pack [button .tb.gain -command snack::mixerDialog -image snackGain -highlightthickness 0 -border $border] -side left
bind .tb.gain <Enter> {SetMsg "Open gain control panel"}

pack [frame .tb.f4 -width 1 -height 20 -highlightth 1] -side left -padx 5
pack [button .tb.zoom -command OpenZoomWindow -image snackZoom -highlightthickness 0 -border $border] -side left
bind .tb.zoom <Enter> {SetMsg "Open zoom window"}

frame .of
pack [canvas .of.c -width $v(width) -height 30 -bg $v(bg)] -fill x -expand true
pack [scrollbar .of.xscroll -orient horizontal -command ScrollCmd] -fill x -expand true
bind .of.xscroll <ButtonPress-1> { set v(scroll) 1 }
bind .of.xscroll <ButtonRelease-1> RePos
bind .of.c <1> {OverPlay %x}

pack [ frame .bf] -side bottom -fill x
entry .bf.lab -textvar v(msg) -width 1 -relief sunken -bd 1 -state disabled
pack .bf.lab -side left -expand yes -fill x

set v(toth) [expr $v(waveh) + $v(spegh) + $v(timeh)+ $v(labelh)]
pack [ frame .cf] -fill both -expand true
pack [ frame .cf.fyc] -side left -anchor n
canvas .cf.fyc.yc2 -height 0 -width $v(yaxisw) -highlightthickness 0
pack [ canvas .cf.fyc.yc -width $v(yaxisw) -height $v(toth) -highlightthickness 0 -bg $v(bg)]

pack [ frame .cf.fc] -side left -fill both -expand true
set c [canvas .cf.fc.c -width $v(width) -height $v(toth) -xscrollcommand [list .cf.fc.xscroll set] -yscrollcommand [list .cf.fc.yscroll set] -closeenough 5 -highlightthickness 0 -bg $v(bg)]
scrollbar .cf.fc.xscroll -orient horizontal -command [list $c xview]
scrollbar .cf.fc.yscroll -orient vertical -command yScroll
#pack .cf.fc.xscroll -side bottom -fill x
#pack .cf.fc.yscroll -side right -fill y
pack $c -side left -fill both -expand true

proc yScroll args {
    global c

    eval .cf.fyc.yc yview $args
    eval $c yview $args
}

$c create rect -1 -1 -1 -1 -tags mfill -fill yellow -stipple gray25
$c create line -1 0 -1 $v(toth) -width 1 -tags [list mark [expr 0 * $v(freq)/$v(pps)] m1] -fill $v(fg)
$c create line -1 0 -1 $v(toth) -width 1 -tags [list mark [expr 0 * $v(freq)/$v(pps)] m2] -fill $v(fg)

bind all <Control-l> {
    set n 0
    if {$labels == {}} return
    while {[lindex [$c coords lab$n] 0] < [expr $v(width) * [lindex [$c xview] 0]]} { incr n }

    $c focus lab$n
    focus $c
    $c icursor lab$n 0
    set i 0
    SetMsg [lindex $labels $i] $i
    SetUndo $labels
}

$c bind text <Control-p> {
    set __x [lindex [%W coords [%W focus]] 0]
    set __y [lindex [%W coords [%W focus]] 1]
    set __n [lindex [$c gettags [$c find closest $__x $__y]] 0]
    PlayNthLab $__n
    break
}

$c bind text <Button-1> {
    %W focus current
    %W icursor current @[$c canvasx %x],[$c canvasy %y]
    set i [lindex [$c gettags [%W focus]] 0]
    SetMsg [lindex $labels $i] $i
    SetUndo $labels
}

event add <<Delete>> <Delete>
catch {event add <<Delete>> <hpDeleteChar>}

$c bind text <<Delete>> {
    if {[%W focus] != {}} {
	%W dchars [%W focus] insert
	SetLabelText [lindex [$c gettags [%W focus]] 0] [$c itemcget [%W focus] -text]
	set i [lindex [$c gettags [%W focus]] 0]
	SetMsg [lindex $labels $i] $i
    }
}

$c bind text <BackSpace> {
    if {[%W focus] != {}} {
	set _tmp [%W focus]
	set _ind [expr [%W index $_tmp insert]-1]
	if {$_ind >= 0} {
	    %W icursor $_tmp $_ind
	    %W dchars $_tmp insert
	    SetLabelText [lindex [$c gettags [%W focus]] 0] [$c itemcget [%W focus] -text]
	    set i [lindex [$c gettags [%W focus]] 0]
	    SetMsg [lindex $labels $i] $i
	}
	unset _tmp _ind
    }
}

$c bind text <Return> {
    %W insert current insert ""
    %W focus {}
}

$c bind text <Enter> {
    %W insert current insert ""
    %W focus {}
}

$c bind text <Control-Any-Key> { break }

$c bind text <Any-Key> {
    if {[%W focus] != {}} {
	%W insert [%W focus] insert %A
	SetLabelText [lindex [$c gettags [%W focus]] 0] [$c itemcget [%W focus] -text]
	set i [lindex [$c gettags [%W focus]] 0]
	SetMsg [lindex $labels $i] $i
    }
    set v(labchanged) 1
}

$c bind text <space> {
    if {[%W focus] != {}} {
	%W insert [%W focus] insert _
	SetLabelText [lindex [$c gettags [%W focus]] 0] [$c itemcget [%W focus] -text]
	set i [lindex [$c gettags [%W focus]] 0]
	SetMsg [lindex $labels $i] $i
    }
}

$c bind text <Key-Right> {
    if {[%W focus] != {}} {
	set __index [%W index [%W focus] insert]
	%W icursor [%W focus] [expr $__index + 1]
	if {$__index == [%W index [%W focus] insert]} {
            set __focus [expr [lindex [$c gettags [%W focus]] 0] + 1]
	    %W focus lab$__focus
	    %W icursor lab$__focus 0
	    set i [lindex [$c gettags [%W focus]] 0]
	    SetMsg [lindex $labels $i] $i
	    while {[expr $v(width) * [lindex [$c xview] 1] -10] < [lindex [%W coords [%W focus]] 0] && [lindex [$c xview] 1] < 1} {
		$c xview scroll 1 unit
	    }
	}
    }
}

$c bind text <Key-Left> {
    if {[%W focus] != {}} {
	set __index [%W index [%W focus] insert]
	%W icursor [%W focus] [expr [%W index [%W focus] insert] - 1]
	if {$__index == [%W index [%W focus] insert]} {
            set __focus [expr [lindex [$c gettags [%W focus]] 0] - 1]
	    %W focus lab$__focus
	    %W icursor lab$__focus end
	    set i [lindex [$c gettags [%W focus]] 0]
	    SetMsg [lindex $labels $i] $i
	    while {[expr $v(width) * [lindex [$c xview] 0] +10] > [lindex [%W coords [%W focus]] 0] && [lindex [$c xview] 0] > 0} {
		$c xview scroll -1 unit
	    }
	}
    }
}

set _mx 1
set _mb 0
#$c bind bound  <B1-Motion> { MoveBoundary %x }
$c bind bound  <ButtonRelease-1> { set _mb 0 ; Redraw quick }
$c bind m1     <B1-Motion> { PutMarker m1 %x %y 1 }
$c bind m2     <B1-Motion> { PutMarker m2 %x %y 1 }
$c bind m1     <ButtonPress-1>   { set _mx 0 }
$c bind m2     <ButtonPress-1>   { set _mx 0 }
$c bind obj    <ButtonPress-1> { PutMarker m1 %x %y 1 }
$c bind obj    <B1-Motion>     { PutMarker m2 %x %y 1 }
$c bind m1     <ButtonRelease-1> { SendPutMarker m1 %x ; set _mx 0 }
$c bind m2     <ButtonRelease-1> { SendPutMarker m2 %x ; set _mx 0 }
$c bind bound  <Any-Enter> { BoundaryEnter %x }
$c bind mark   <Any-Enter> { MarkerEnter %x }
$c bind bound  <Any-Leave> { BoundaryLeave %x }
$c bind mark   <Any-Leave> { MarkerLeave %x }

bind $c <ButtonPress-1>   {
    if {%y > [expr $v(waveh)+$v(spegh)+$v(timeh)]} {
    } else {
	PutMarker m1 %x %y 1
	SendPutMarker m1 %x
	set _mx 1
    }
}

bind $c <ButtonRelease-1> {
    set _mb 0
    if {%y > [expr $v(waveh)+$v(spegh)+$v(timeh)]} {
	focus %W
	if {[%W find overlapping [expr [$c canvasx %x]-2] [expr [$c canvasy %y]-2] [expr [$c canvasx %x]+2] [expr [$c canvasy %y]+2]] == {}} {
	    %W focus {}
	}
    } else {
	PutMarker m2 %x %y 1
	SendPutMarker m2 %x
	set _mx 1
    }
}

bind $c <Motion> { PutCrossHairs %x %y }
bind $c <Leave>  {
    $c coords ch1 -1 -1 -1 -1
    $c coords ch2 -1 -1 -1 -1
}
bind $c <3>      { PopUpMenu %X %Y %x %y }

bind .cf.fc.xscroll <ButtonRelease-1> SendXScroll
bind .bf.lab <Any-KeyRelease> { InputFromMsgLine %K }
bind all <Control-c> Exit
wm protocol . WM_DELETE_WINDOW Exit
bind .cf.fc.c <Configure> { if {"%W" == ".cf.fc.c"} Reconf }
bind $c <F1> { PlayToCursor %x }
bind $c <2>  { PlayToCursor %x }
focus $c

if [info exists package_sphere] {
    snack::addExtTypes {{NIST .wav} {TIMIT .phn} {MIX .smp.mix} {HTK .lab} {WAVES .lab}}
    snack::addLoadTypes {{{NIST Wav Files} {.wav}} {{MIX Files} {.mix}} {{HTK Label Files} {.lab}} {{TIMIT Label Files} {.phn}} {{TIMIT Label Files} {.wrd}} {{Waves Label Files} {.lab}}} {NIST MIX HTK TIMIT WAVES}
} else {
    snack::addExtTypes {{TIMIT .phn} {MIX .smp.mix} {HTK .lab} {WAVES .lab}}
    snack::addLoadTypes {{{MIX Files} {.mix}} {{HTK Label Files} {.lab}} {{TIMIT Label Files} {.phn}} {{TIMIT Label Files} {.wrd}} {{Waves Label Files} {.lab}}} {MIX HTK TIMIT WAVES}
}

proc GetOpenFileName {} {
    global f v

    if {$v(smpchanged) || $v(labchanged)} {
	if {[tk_messageBox -message "You have unsaved changes.\n Do you \
		really want to close?" -type yesno \
		-icon question] == "no"} return
    }

    set gotfn [snack::getOpenFile -initialdir $f(spath) \
	    -initialfile $f(sndfile) -format $v(smpfmt)]

    # Ugly hack for Tk8.0
    if {$gotfn != ""} {
	set tmp [file split $gotfn]
	if {[lindex $tmp 0] == [lindex $tmp 1]} {
	    set tmp [lreplace $tmp 0 0]
	    set gotfn [eval file join $tmp]
	}
    }
    update
    if [string compare $gotfn ""] {
	OpenFiles $gotfn
    }
}

proc GetSaveFileName title {
    global f v labels

    if {$labels != {} && [string compare $title "Save sample file"] != 0} {  
	switch $v(labfmt) {
	    MIX {
		snack::addSaveTypes {{{MIX Files} {.mix}}} {MIX}
	    }
	    HTK {
		snack::addSaveTypes {{{HTK Label Files} {.lab}}} {HTK}
	    }
	    TIMIT {
		snack::addSaveTypes {{{TIMIT Label Files} {.phn}} {{TIMIT Label Files} {.wrd}}} {TIMIT}
	    }
	    WAVES {
		snack::addSaveTypes {{{Waves Label Files} {.lab}}} {WAVES}
	    }
	    default
	}
	set gotfn [snack::getSaveFile -initialdir $f(lpath) -initialfile $f(labfile) -format $v(labfmt) -title $title]
 } else {
	set gotfn [snack::getSaveFile -initialdir $f(spath) -initialfile $f(sndfile) -format $v(smpfmt) -title $title]
    }
#    set tmp [string trimright $f(lpath) /].
#    if {[regexp $tmp $gotfn] == 1 && $tmp != "."} {
#	return ""
#    }
    update
    return $gotfn
}

proc SaveAs {} {
    set gotfn [GetSaveFileName ""]
    if {[string compare $gotfn ""] != 0} {
	SaveFile $gotfn
    }
}

proc Save {} {
    global f v

    set fn $f(spath)$f(sndfile)
    if {[string compare $f(spath)$f(sndfile) ""] == 0} {
	set fn [GetSaveFileName "Save sample file"]
    }
    if {$fn != "" && $v(smpchanged)} {
	SaveFile $fn
    }
    if $v(labchanged) {
	set fn $f(lpath)$f(labfile)
	if {[string compare $f(lpath)$f(labfile) ""] == 0} {
	    set fn [GetSaveFileName "Save label file"]
	}
	if {$fn != ""} {
	    SaveFile $fn
	}
    }
}

proc SaveFile {{fn ""}} {
    global f v labels

    SetCursor watch
    set strip_fn [lindex [file split [file rootname $fn]] end]
    set ext  [file extension $fn]
    set path [file dirname $fn]/
    if {$path == "./"} { set path ""}

    if ![IsLabelFile $fn] {
	if {$v(linkfile)} {
	    if [string match [snd cget -file] _xs[pid].wav] {
		file rename -force _xs[pid].wav $fn
	    } else {
		file copy -force [snd cget -file] $fn
	    }
	    snd configure -file $fn
	} else {
	    snd write $fn
	}
	set v(smpchanged) 0
	wm title . "xs: $fn"
	set f(spath) $path
	set f(sndfile) $strip_fn$ext
    } elseif {$labels != {}} {
	SaveLabelFile $labels $fn
	set v(labchanged) 0
	wm title . "xs: $f(spath)$f(sndfile) - $fn"
	set f(lpath) $path
	set f(labfile) $strip_fn$ext
    }
    SetCursor ""
}

proc IsLabelFile fn {
    set ext [file extension $fn]
    if {$ext == ".lab"} { return 1 }
    if {$ext == ".mix"} { return 1 }
    if {$ext == ".phn"} { return 1 }
    if {$ext == ".wrd"} { return 1 }
    return 0
}

proc OpenFiles fn {
    global c labels v f

    SetCursor watch
    set strip_fn [lindex [file split [file rootname $fn]] end]
    set ext  [file extension $fn]
    set path [file dirname $fn]/
    if {$path == "./"} { set path ""}

    if [IsLabelFile $fn] {
	set type "lab"
	set f(lpath) $path
    } else {
	set type "smp"
	set f(spath) $path
    }

    switch $ext {
	.mix {
	    set f(labfile) "$strip_fn.mix"
	    set v(labfmt) MIX
	    if $v(autoload) {
		set f(sndfile) "$strip_fn"
		if {$f(spath) == ""} { set f(spath) $f(lpath) }
		if {[file exists $f(spath)$f(sndfile)] == 0} {
		    set f(sndfile) "$strip_fn.smp"
		}
	    }
	}
	.lab {
	    set f(labfile) "$strip_fn.lab"
	    if {$v(smpfmt) == "SD"} {
		set v(labfmt) WAVES
		set v(labalign) e
		if $v(autoload) {
		    set f(sndfile) "$strip_fn.d"
		    if {$f(spath) == ""} { set f(spath) $f(lpath) }
		}
	    } else {
		set v(labfmt) HTK
		if $v(autoload) {
		    set f(sndfile) "$strip_fn.smp"
		    if {$f(spath) == ""} { set f(spath) $f(lpath) }
		}
	    }
	}
	.phn {
	    set f(labfile) "$strip_fn.phn"
	    set v(labfmt) TIMIT
	    if $v(autoload) {
		set f(sndfile) "$strip_fn.wav"
		if {$f(spath) == ""} { set f(spath) $f(lpath) }
	    }
	}
	.wrd {
	    set f(labfile) "$strip_fn.wrd"
	    set v(labfmt) TIMIT
	    if $v(autoload) {
		set f(sndfile) "$strip_fn.wav"
		if {$f(spath) == ""} { set f(spath) $f(lpath) }
	    }
	}
	.smp {
	    set f(sndfile) "$strip_fn.smp"
	    set v(labfmt) MIX
	    if $v(autoload) {
		set f(labfile) "$strip_fn.smp.mix"
		if {$f(lpath) == ""} { set f(lpath) $f(spath) }
		if {[file exists $f(lpath)$f(labfile)] == 0} {
		    set f(labfile) "$strip_fn.mix"
		}
	    }
	}
	.wav {
	    set f(sndfile) "$strip_fn.wav"
	    set v(labfmt) TIMIT
	    if $v(autoload) {
		set f(labfile) "$strip_fn.phn"
		if {$f(lpath) == ""} { set f(lpath) $f(spath) }
	    }
	}
	.sd {
	    set f(sndfile) "$strip_fn.sd"
	    set v(labfmt) WAVES
	    if $v(autoload) {
		set f(labfile) "$strip_fn.lab"
		if {$f(lpath) == ""} { set f(lpath) $f(spath) }
	    }
	}
	default {
	    if {$type == "smp"} {
		set f(sndfile) "$strip_fn$ext"
		if $v(autoload) {
		    set f(labfile) "$strip_fn$ext.mix"
		    set v(labfmt) MIX
		    if {$f(lpath) == ""} { set f(lpath) $f(spath) }
		}
	    } else {
		set f(labfile) "$strip_fn$ext"
		if $v(autoload) {
		    set f(sndfile) "$strip_fn.smp"
		    if {$f(spath) == ""} { set f(spath) $f(lpath) }
		}
	    }
	}
    }

    if {($v(autoload) == 1) || ($type == "smp")} {
	$c delete wave speg
	.of.c delete overwave
	catch {.sect.c delete sect}
	StopPlay
	if {$v(linkfile)} {
	    if {[string length $f(byteOrder)] == 0} {
		if [catch {snd configure -file $f(spath)$f(sndfile) -skip $f(skip) -guessproperties $f(guessraw)} ret] {
		    SetMsg "$ret"
		    return
		}
		set v(smpfmt) [lindex [snd info] 6]
	    } else {
		if [catch {snd configure -file $f(spath)$f(sndfile) -skip $f(skip) -byteorder $f(byteOrder) -guessproperties $f(guessraw)} ret] {
		    SetMsg "$ret"
		    return
		}
		set v(smpfmt) [lindex [snd info] 6]
	    }
	} else {
	    if {[string length $f(byteOrder)] == 0} {
		if [catch {set v(smpfmt) [snd read $f(spath)$f(sndfile) -skip $f(skip) -guessproperties $f(guessraw)]} ret] {
		    SetMsg "$ret"
		    return
		}
	    } else {
		if [catch {set v(smpfmt) [snd read $f(spath)$f(sndfile) -skip $f(skip) -byteorder $f(byteOrder) -guessproperties $f(guessraw)]} ret] {
		    SetMsg "$ret"
		    return
		}
	    }
	}
	set v(freq) [snd cget -frequency]
	set v(sfmt) [snd cget -format]
	set v(chan) [snd cget -channels]
	set v(startsmp) 0
	if {[snd cget -channels] == 1} {
	    snack::menuEntryOff Options {View Channel}
	    set v(vchan) -1
	} else {
	    snack::menuEntryOn Options {View Channel}
	}
	set v(smpchanged) 0
	snack::menuEntryOff Edit Undo
	.tb.undo config -state disabled
	if {![regexp $v(freq) [audio frequencies]] || \
		![regexp $v(sfmt) [audio formats]]} {
	    tk_messageBox -icon warning -type ok -message "You need to \
		    convert this sound\nif you want to play it"
	}
    }
    if {($v(autoload) == 1) || ($type == "lab")} {
	set labels [OpenLabelFile $f(lpath)$f(labfile)]
	if {$labels == {}} { set f(labfile) "" }
    }
    if {$labels == {}} {
	wm title . "xs: $f(spath)$f(sndfile)"
    } else {
	wm title . "xs: $f(spath)$f(sndfile) - $f(lpath)$f(labfile)"
    }

    if {[snd length -units seconds] > 50 && $v(pps) > 100} {
	set v(pps) [expr $v(pps)/10]
    }
    if {[snd length -units seconds] < 50 && $v(pps) < 100} {
	set v(pps) [expr $v(pps)*10]
    }
    wm geometry . {}
    Redraw
    event generate .cf.fc.c <Configure>
    SetMsg [InfoStr nopath]
#    MarkAll
}

proc Link2File {} {
    global f v

    StopPlay
    if {$v(smpchanged)} {
	if {[tk_messageBox -message "You have unsaved changes.\n Do you \
		really want to loose them?" -type yesno \
		-icon question] == "no"} return
    }
    if {$v(linkfile)} {
	.of.c delete overwave
	catch {.sect.c delete sect}
	if {$f(sndfile) == ""} {
	    snd configure -file _xs[pid].wav
	} else {
	    snd configure -file $f(spath)$f(sndfile)
	}
	cbs configure -file ""
	snack::menuEntryOff Edit Cut
	snack::menuEntryOff Edit Copy
	snack::menuEntryOff Edit Paste
	snack::menuEntryOff Edit Crop
	snack::menuEntryOff Edit Reverse
	snack::menuEntryOff Audio Conversions
#	if $v(recording) {
#	    set v(recording) 0
#	    ToggleRecording
#	}
    } else {
	if {$f(sndfile) == ""} {
	    snd config -load ""
	} else {
	    snd config -load $f(spath)$f(sndfile)
	}
	cbs config -load ""
	snack::menuEntryOn Edit Cut
	snack::menuEntryOn Edit Copy
	snack::menuEntryOn Edit Paste
	snack::menuEntryOn Edit Crop
	snack::menuEntryOn Edit Reverse
	snack::menuEntryOn Audio Conversions
    }
}

proc OpenLabelFile fn {
    global f v undo

    if [catch {open $fn} in] {
	SetMsg $in
	return {}
    } else {
	if [catch {set labelfile [read $in]}] { return {} }
	set l {}
	set undo {}
	set v(labchanged) 0
	snack::menuEntryOff Edit Undo
	.tb.undo config -state disabled
	close $in
	switch $v(labfmt) {
	    TIMIT -
	    HTK {
		foreach row [split $labelfile \n] {
		    set rest ""
		    if {[scan $row {%d %d %s %[^§]} start stop label rest] >= 3} {
			lappend l "$start\n$stop\n$label\n$rest"
		    }
		}
	    }
	    MIX {
		set f(header) ""
		set getHead 1
		foreach row [split $labelfile \n] {
		    if [string match FR* $row] {
			set getHead 0
			if {$v(iso)} {
			    regsub -all {\]} $row Å t1
			    regsub -all {\[} $t1 Ä t2
			    regsub -all {\\} $t2 Ö row
			}
			set rest ""
			if {[scan $row {%s %d %s %[^§]} junk start label rest] >= 3} {
			    lappend l "$start\n$label\n$rest"
			}
		    } else {
			if {$getHead == 1} {
			    set f(header) [lappend f(header) $row]
			}
		    }
		}
	    }
	    WAVES {
		set f(header) ""
		set getHead 1
		foreach row [split $labelfile \n] {
		    if {$getHead == 1} {
			set f(header) [lappend f(header) $row]
			if [string match # $row] { set getHead 0 }
			continue
		    }
		    set rest ""
		    if {[scan $row {%f %d %s %[^§]} end color label rest] >= 3} {
			lappend l "$end\n$color\n$label\n$rest"
		    }
		}
	    }
	}
    }
    return $l
}

proc SaveLabelFile { labels fn } {
    global f v

    if {$fn == "" || [regexp /$ $fn] == 1 || $labels == {}} return
    set f(labfile) [lindex [file split $fn] end]
    set f(lpath) [file dirname $fn]/
    catch {file copy $fn $fn~}
    if [catch {open $fn w} out] {
	SetMsg $out
        return
    } else {
	set v(labchanged) 0
	fconfigure $out -translation {auto lf}
	switch $v(labfmt) {
	    TIMIT -
	    HTK {
		foreach row $labels {
		    puts $out [join $row " "]
		}
	    }
	    MIX {
		if {$f(header) != ""} {
		    puts $out [join $f(header) \n]
		} else {
		    puts $out "NOLABELS\nTEXT: \nCT 1"
		}
		foreach row $labels {
		    if {$v(iso)} {
			regsub -all Å $row {]} t1
			regsub -all Ä $t1 {[} t2
			regsub -all Ö $t2 {\\} row
		    }
		    set t4 [split $row \n]
		    set lab [lindex $t4 1]
		    if {[string compare $lab "OK"] == 0} {
		    } elseif {[string index $lab 0] == "$"} {
		    } elseif {[string index $lab 0] == "#"} {
		    } else {
			set t4 [lreplace $t4 1 1 "\$$lab"]
		    }
		    set t5 [join $t4 "\t"]
		    puts $out "FR\t$t5"
		}
	    }
	    WAVES {
		if {$f(header) != ""} {
		    puts $out [join $f(header) \n]
		} else {
		    set name [lindex [file split [file rootname $fn]] end]
		    set date [clock format [clock seconds] -format "%a %b %d %H:%M:%S %Y"]
		    puts $out "signal $name"
		    puts $out "type 0\ncolor 121"
		    puts $out "comment created using xs $date"
		    puts $out "font -misc-*-bold-*-*-*-15-*-*-*-*-*-*-*"
		    puts $out "separator ;\nnfields 1\n#"
		}
		foreach row $labels {
		    set rest ""
		    scan $row {%f %d %s %[^§]} end color label rest
		    puts $out [format "    %.6f  %d %s %s" $end $color $label $rest]
		}
	    }
	}
	close $out
    }
    SetMsg "Saved: $fn"
}

proc SaveMark {} {
    global f v labels

    if {$v(linkfile)} {
	tk_messageBox -icon warning -type ok -message "This function\
		currently not available when using link to disk file."
	return
    }

    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]

    set gotfn [snack::getSaveFile -initialdir $f(spath) -format $v(smpfmt)]

    if [string compare $gotfn ""] {
	SetMsg "Saving range: $start $end"

	set ext [file extension $gotfn]
	set root [file rootname $gotfn]
	if {$root == $gotfn} {
	    set fn $root[file extension $f(sndfile)]
	} else {
	    set fn $gotfn
	}

	snd write $fn -start $start -end $end

	if {$labels != {}} {
	    set fn $root[file extension $f(labfile)]
	    switch $v(labfmt) {
		WAVES -
		HTK {
		    SaveLabelFile [CropLabels [Marker2Time m1] [Marker2Time m2]] $fn
		}
		TIMIT -
		MIX {
		    SaveLabelFile [CropLabels $start $end] $fn
		}
	    }
	}
    }
}

proc Close {} {
    global labels f v c

    if {$v(smpchanged) || $v(labchanged)} {
	if {[tk_messageBox -message "You have unsaved changes.\n Do you \
		really want to close?" -type yesno \
		-icon question] == "no"} return
    }
    StopPlay
    set labels {}
    set v(smpchanged) 0
    set v(labchanged) 0
    set undo {}
    snack::menuEntryOff Edit Undo
    .tb.undo config -state disabled
    set f(labfile) ""
    set f(sndfile) ""
    wm title . "xs:"
    if {$v(linkfile)} {
	catch {file delete -force _xs[pid].wav}
	snd configure -file _xs[pid].wav
    } else {
	snd flush
    }
    Redraw
}

proc Exit {} {
    global v

    if {$v(smpchanged) || $v(labchanged)} {
	if {[tk_messageBox -message \
		"You have unsaved changes.\n Do you really want to quit?" \
		-type yesno -icon question] == "no"} {
	    return
	}
    }
    catch {file delete -force _xs[pid].wav}
    exit
}

proc OpenGetURLWindow {} {
    global f v

    if {$v(linkfile)} {
	tk_messageBox -icon warning -type ok -message "This function not \
		available\nwhen using link to disk file."
	return
    }

    set w .geturl
    catch {destroy $w}
    toplevel $w
    wm title $w {Get URL}
    wm geometry $w [xsGetGeometry]

    set f(url) $f(http)
    pack [frame $w.f]
    pack [label $w.f.l -text {Enter the World Wide Web location (URL):}]
    pack [entry $w.f.e -width 60 -textvar f(url)]
    pack [frame $w.f2]
    pack [button $w.f2.b1 -text Get -command GetURL] -side left
    bind $w.f.e <Key-Return> GetURL
    pack [button $w.f2.b2 -text Stop -command StopURL] -side left
    pack [button $w.f2.b3 -text Close -command [list destroy $w]] -side left
}

proc GetURL {} {
    global c f

    SetCursor watch
    $c delete wave speg tran
    StopPlay
    StopURL
    set f(urlToken) [::http::geturl $f(url) -command URLcallback -progress Progress]
    set c .cf.fc.c
    SetMsg "Fetching: $f(url)"
}

proc Progress {token total current} {
    if {$total > 0} {
	set p [expr int(100 * $current/$total)]
	SetMsg "Fetched $current bytes ($p%)"
    } else {
	SetMsg "Fetched $current bytes"
    }
}

proc URLcallback token {
    global f v labels
    upvar #0 $token state

    SetCursor ""
    if {$state(status) != "ok"} {
	return
    }
    if {[string match *200* [::http::code $token]] == 1} {
	snd data [::http::data $token]
	set f(sndfile) ""
	set f(labfile) ""
	set v(freq) [snd cget -frequency]
	set v(sfmt) [snd cget -format]
	set v(startsmp) 0
	set labels {}
	wm title . "xs: $f(url)"
	Redraw
	event generate .cf.fc.c <Configure>
	MarkAll
	SetMsg [InfoStr nopath]
    } else {
	SetMsg [::http::code $token]
    }
    set f(urlToken) ""
}

proc StopURL {} {
    global f v
    
    if {$f(urlToken) != ""} {
	::http::reset $f(urlToken)
    }
    set f(urlToken) ""
    SetMsg "Transfer interrupted."
    SetCursor ""
}

proc Crop {} {
    global labels v

    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]
    if {$start == $end} return
    SetMsg "Cropping to range: $start $end"

    cbs copy snd -end [expr $start - 1]
    cbs insert snd [cbs length] -start [expr $end + 1]
    snd crop $start $end

    set v(undoc) "snd insert cbs 0 -end [expr $start-1];snd insert cbs [expr $end+1] -start $start"
    set v(redoc) "snd crop $start $end"
    set v(smpchanged) 1

    if {[llength $labels] != 0} {
	switch $v(labfmt) {
	    WAVES -
	    HTK {
		set labels [CropLabels [Marker2Time m1] [Marker2Time m2]]
	    }
	    TIMIT -
	    MIX {
		set labels [CropLabels $start $end]
	    }
	}
	set v(labchanged) 1
    }
    PutMarker m1 [DTime2Time 0.0] 0 0
    PutMarker m2 [DTime2Time [snd length -units seconds]] 0 0
    snack::menuEntryOn Edit Undo
    .tb.undo config -state normal
    DrawOverAxis
    Redraw
}

proc Reverse {} {
    global labels v

    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]
    if {$start == $end} return
    SetMsg "Reversing range: $start $end"

    snd reverse -start $start -end $end

    set v(undoc) "snd reverse -start $start -end $end"
    set v(redoc) "snd reverse -start $start -end $end"
    set v(smpchanged) 1

    snack::menuEntryOn Edit Undo
    .tb.undo config -state normal
    cbs length 1 ;# hack to fool Undo
}

proc Cut {} {
    global c v

    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]
    if {$start == $end} return
    SetMsg "Cutting range: $start $end"
    cbs copy snd -start $start -end $end
    snd cut $start $end
    set v(undoc) "snd insert cbs $start"
    set v(redoc) "snd cut $start $end"

    PutMarker m2 [Marker2Time m1] 0 0
    set v(smpchanged) 1
    snack::menuEntryOn Edit Undo
    .tb.undo config -state normal
    DrawOverAxis
    Redraw
}

proc Copy {} {
    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]
    if {$start == $end} return
    SetMsg "Copying range: $start $end"
    cbs copy snd -start $start -end $end
}

proc Paste {} {
    global c v

    set start [Marker2Sample m1]
    set startt [Marker2Time m1]
    if {$start > [snd length]} {
	set start [snd length]
	set startt [snd length -units seconds]
    }
    SetMsg "Inserting at: $start"
    snd insert cbs $start

    set tmp [expr $start + [cbs length] - 1]
    set v(undoc) "snd cut $start $tmp"
    set v(redoc) "snd insert cbs $start"

    PutMarker m2 [expr $startt + [DTime2Time [cbs length -units seconds]]] 0 0
    set v(smpchanged) 1
    snack::menuEntryOn Edit Undo
    .tb.undo config -state normal
    DrawOverAxis
    Redraw
}

proc SendXScroll {} {
    global c v

    if $v(slink) {
	foreach prg [winfo interps] {
	    if [regexp .*xs.* $prg] {
		if {[winfo name .] != $prg} {
		    send $prg RecvXScroll [Coord2Time [expr [lindex [.cf.fc.xscroll get] 0] * $v(width)]]
		}
	    }
	}
    }
}

proc RecvXScroll a {
    global c v

    set f [Time2Coord [expr double($a / $v(width))]]
    eval $c xview moveto $f
}

proc Redraw args {
    global c labels f v debug

    SetCursor watch
    set length [snd length]
    if {$args != "quick"} {
	$c delete obj
	$c config -bg $v(bg)
	.cf.fyc.yc config -bg $v(bg)
	.of.c config -bg $v(bg)
	if {$length == 0} { set length 1 }
	set v(endsmp) [expr $v(startsmp) + $v(freq) * $v(scrw) / $v(pps)]
	if {$v(endsmp) > $length} {
	    set v(endsmp) $length
	}

	if {[expr int(double($length * $v(pps)) / $v(freq))] < $v(scrw)} {
	    if [winfo exist .of] { pack forget .of }
	    set v(startsmp) 0
	    set v(endsmp) $length
	} else {
	    pack .of -side top -fill x -before .cf
	    if {$::tcl_platform(platform) == "windows"} {
		DrawOverAxis
	    }
	}
	.of.xscroll set [expr double($v(startsmp)) / $length] [expr double($v(endsmp)) / $length]

	.cf.fyc.yc delete axis
	if {$v(waveh) > 0} {
	    if {$v(linkfile) && $f(sndfile) != ""} {
		snack::openShapeMsgBox $f(spath)$f(sndfile)
		$c create waveform 0 0 -sound snd -height $v(waveh) \
			-pixels $v(pps) -tags [list obj wave] \
			-start $v(startsmp) -end $v(endsmp) \
			-channel $v(vchan) -debug $debug -fill $v(fg) \
			-shapefile [file rootname $f(spath)$f(sndfile)].shape
		snack::closeShapeMsgBox $f(spath)$f(sndfile)
	    } else {
		$c create waveform 0 0 -sound snd -height $v(waveh) \
			-pixels $v(pps) -tags [list obj wave] \
			-start $v(startsmp) -end $v(endsmp) \
			-channel $v(vchan) -debug $debug -fill $v(fg)
	    }
	    $c lower wave
	    .cf.fyc.yc create text $v(yaxisw) 2 -text [snd max]\
		    -font $v(sfont) -anchor ne -tags axis -fill $v(fg)
	    .cf.fyc.yc create text $v(yaxisw) $v(waveh) -text [snd min]\
		    -font $v(sfont) -anchor se -tags axis -fill $v(fg)
	    .cf.fyc.yc create line 0 [expr $v(waveh)+0] $v(yaxisw) \
		    [expr $v(waveh)+0] -tags axis -fill $v(fg)  
	}
	if {$v(topfr) > [expr $v(freq)/2]} {
	    set v(topfr) [expr $v(freq)/2]
	}
	if {$v(spegh) > 0} {
	    set v(winlen) [expr int($v(freq) / $v(anabw))]
	    $c create spectrogram 0 $v(waveh) -sound snd -fftlen $v(fftlen) \
		    -winlen $v(winlen) -height $v(spegh) -pixels $v(pps) \
		    -preemph $v(preemph) -topfr $v(topfr) -tags [list obj speg] \
		    -start $v(startsmp) -end $v(endsmp)\
		    -contrast $v(contrast) -brightness $v(brightness)\
		    -gridtspacing $v(gridtspacing) \
		    -gridfspacing $v(gridfspacing) -channel $v(vchan) \
		    -colormap $v($v(cmap)) -gridcol $v(gridcolor) -debug $debug
	    $c lower speg
	    snack::frequencyAxis .cf.fyc.yc 0 $v(waveh) $v(yaxisw) $v(spegh)\
		    -topfrequency $v(topfr) -tags axis -fill $v(fg)\
		    -font $v(sfont)
	    .cf.fyc.yc create line 0 [expr $v(spegh) + $v(waveh)+0] \
		    $v(yaxisw) [expr $v(spegh) + $v(waveh)+0] -tags axis\
		    -fill $v(fg)
	}

	set v(width) [expr int($v(pps) * double($v(endsmp) - $v(startsmp)) / $v(freq))]
	if {$v(width) == 0} { set v(width) 600 } 
	$c create line 0 0 $v(width) 0 -tags obj -fill $v(fg)
	$c create line 0 $v(waveh) $v(width) $v(waveh) -tags obj -fill $v(fg)

    }

    $c delete tran axis
    set y [expr $v(waveh) + $v(spegh)]
    $c create line 0 $y $v(width) $y -tags axis -fill $v(fg)

    if {[expr $v(pps) * $v(npt)] > 100000} {
	set v(npt) 1
    }

    set v(npt) [snack::timeAxis $c 0 $y $v(width) $v(timeh) $v(pps) $v(npt)\
	    -tags axis -starttime [expr double($v(startsmp)) / $v(freq)]\
	    -fill $v(fg) -font $v(sfont)]
    incr y $v(timeh)
    $c create line 0 $y $v(width) $y -tags axis -fill $v(fg)

    .cf.fyc.yc configure -height $y
    if {$v(npt) < 1000} {
	set time {t [ms]}
    } else {
	set time {t [s]}
    }
    .cf.fyc.yc create text 5 [expr $v(waveh) + $v(spegh) + 2] -text $time \
	    -font $v(sfont) -anchor nw -tags axis -fill $v(fg)

    if $v(ipa) {
	incr y [DrawLabels $y $labels ipa]
    }
    incr y [DrawLabels $y $labels lab]

    foreach p $v(plugins) {
	incr y [namespace inscope $p Redraw $y]
    }

    set v(toth) $y
    $c configure -height $v(toth) -width $v(width) -scrollregion "0 0 $v(width) $v(toth)"
    .cf.fyc.yc configure -height $v(toth) -scrollregion "0 0 $v(yaxisw) $v(toth)"

# Someday in a perfect universe

    if {$::tcl_platform(os) == "Linux"} {
	set maxw [lindex [wm maxsize .] 0]
	if {$v(width) > $maxw} {
	    if [winfo exist .of] {
		. config -width $maxw -height [expr $v(toth) + 130]
	    } else {
		. config -width $maxw -height [expr $v(toth) + 40]
	    }
	    pack propagate . 0
	} else {
	    pack propagate . 1
	}
    }
    if {$::tcl_platform(platform) == "windows"} {
	set maxw [lindex [wm maxsize .] 0]
	if {$v(width) > $maxw} {
	    if {[expr int(double($length * $v(pps)) / $v(freq))] >= $v(scrw)} {
		wm geometry . [expr $maxw - 15]x[expr $v(toth) + 120]
	    } else {
		wm geometry . [expr $maxw - 15]x[expr $v(toth) + 70]
	    }
	}
    }

    catch {PutMarker m1 [Marker2Time m1] 0 0}
    catch {PutMarker m2 [Marker2Time m2] 0 0}
    SetCursor ""
}

proc DrawLabels {y labels type} {
    global c v f ipa

    if {[llength $labels] == 0} {
	return 0
    } else {
	$c create line 0 [expr $y + $v(labelh)]	[expr $v(width) -1] \
		[expr $y + $v(labelh)] -tags obj -fill $v(fg)
	set start 0
	set end   0
	set label ""
	set i 0
	foreach row $labels {
	    switch $v(labfmt) {
		TIMIT -
		HTK {
		    scan $row {%d %d %s} start end label
		    set lx [Time2Coord $start]
#		    if {!$v(zerolabs) && $end == $start} continue
		}
		MIX {
		    scan $row {%d %s} start label
		    set lx [Time2Coord $start]
		    set end [Coord2Time $v(width)]
		    scan [lindex $labels [expr $i+1]] {%d} end
		}
		WAVES {
		    scan $row {%f %d %s} end color label
		    set lx [Time2Coord $end]
		    set start 0
		    scan [lindex $labels [expr $i-1]] {%f} start
		}
	    }
	    if {$lx >= 0 && $lx <= $v(width)} {
		if {$v(labalign) == "c"} {
		    set tx [Time2Coord [expr ($start+$end)/2]]
		} elseif {$v(labalign) == "w"} {
		    set tx [expr [Time2Coord $start] + 2] 
		} else {
		    set tx [Time2Coord $end]
		}
		if {$type == "lab"} {
		    $c create text $tx [expr $y+12] -text $label\
			    -font $v(font) -anchor $v(labalign)\
			    -tags [list $i obj text lab$i tran] -fill $v(fg)
		    $c create line $lx $y $lx [expr $y+$v(labelh)] \
			    -tags [list b$i obj bound tran] -fill $v(fg)
		} else {
		    if {$v(labfmt) == "MIX"} {
			regsub -all {\$} $label "" t1
			regsub -all {\"} $t1    "" t2
			regsub -all # $t2       "" t3
			regsub -all {\`} $t3    "" t4
			regsub -all {\'} $t4    "" tmp
			set label $tmp
		    }
#		catch {$c create image $tx [expr $y+2] \
#		   -image [image create photo -file $f(ipapath)/$ipa($label)] \
#		   -anchor n -tags [list obj tran]}
		    if {$::tcl_platform(platform) == "windows"} {
			$c create text $tx [expr $y+12] \
				-text $label -font IPAKiel -fill $v(fg)\
				-anchor $v(labalign) -tags [list obj tran]
		    } else {
			catch {$c create bitmap $tx [expr $y+2] \
				-bitmap @$f(ipapath)/$ipa($label) \
				-anchor n -tags [list obj tran]}
		    }
		    $c create line $lx [expr $y] $lx [expr $y+$v(labelh)] \
			    -tags [list obj tran] -fill $v(fg)
		}
	    }
	    incr i
	}
    }
    return $v(labelh)
}

proc ScrollCmd args {
    global v

    if {[lindex $args 0] == "moveto"} {
	set delta [expr [lindex [.of.xscroll get] 1] - [lindex [.of.xscroll get] 0]]
	set pos [lindex $args 1]
	if {$pos < 0.0} { set pos 0.0 }
	if {$pos > [expr 1.0 - $delta]} { set pos [expr 1.0 - $delta] }
	.of.xscroll set $pos [expr $pos + $delta]
    } elseif {[lindex $args 0] == "scroll" && $v(scroll) == 1} {
	set pos [expr double($v(startsmp)) / [snd length]]
	set delta [expr double($v(endsmp)) / [snd length] - $pos]
	if {[lindex $args 1] > 0} {
            set pos [expr $pos + $delta]
	    if {$pos > [expr 1.0 - $delta]} { set pos [expr 1.0 - $delta] }
	}
	if {[lindex $args 1] < 0} {
            set pos [expr $pos - $delta]
            if {$pos < 0.0} { set pos 0.0 }
	}
	.of.xscroll set $pos [expr $pos + $delta]
	set v(scroll) 0
    }
}

proc RePos args {
    global v c

    set v(startsmp) [expr int ([lindex [.of.xscroll get] 0] * [snd length])]
    set v(endsmp)   [expr int ([lindex [.of.xscroll get] 1] * [snd length])]
    $c xview moveto 0
    Redraw
}

proc DrawOverAxis {} {
    global v

    set totw [winfo width .]
    set scrh [winfo height .of.xscroll]
    set width [expr $totw - 2 * $scrh]
    set length [snd length -units seconds]
    set v(opps) [expr 1000.0*$width/$length]
    .of.c delete overaxis
    snack::timeAxis .of.c $scrh 20 $width 11 $v(opps) 1 -tags overaxis -fill $v(fg)
}

proc OverPlay x {
    global v
    
    set start [expr int($v(freq)*(($x - [winfo height .of.xscroll]) * 1000 / $v(opps)))]
    set end   [snd length]
    Stop snd
    if {$start < 0} { set start 0 }
    set v(s0) $start
    set v(s1) $end
    Play snd $start $end
    .of.c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    after cancel PutPlayMarker
    after 50 PutPlayMarker
}

proc Reconf {} {
    global c v f debug

    set dox 0
    set doy 0
    if {[$c xview] == "0 1"} { set dox 1 }
    if {[$c yview] == "0 1"} { set doy 1 }

    if {$dox} {
	pack forget .cf.fc.xscroll
	pack forget .cf.fyc.yc2
    } else {
	pack .cf.fc.xscroll -side bottom -fill x -before $c
	.cf.fyc.yc2 config -height [winfo height .cf.fc.xscroll]
	pack .cf.fyc.yc2 -side bottom -fill x -before .cf.fyc.yc
    }
    if {$doy} {
	pack forget .cf.fc.yscroll
    } else {
	pack .cf.fc.yscroll -side right -fill y -before $c
    }

    set ww [.of.c itemcget overwave -width]
    set v(scrh) [winfo height .of.xscroll]
    set totw [expr [winfo width .] - 2 * $v(scrh)]
    if {$ww != $totw} {
	.of.c delete overwave
        if {$v(linkfile) && $f(sndfile) != ""} {
	    .of.c create waveform $v(scrh) 0 -sound snd -height 20 \
		    -width $totw -tags overwave -fill $v(fg) -debug $debug \
		    -shapefile [file rootname $f(spath)$f(sndfile)].shape
	} else {
	    .of.c create waveform $v(scrh) 0 -sound snd -height 20 \
		    -width $totw -tags overwave -fill $v(fg) -debug $debug
	}
	.of.c create rectangle -1 -1 -1 -1 -tags mark -fill yellow -stipple gray25
    }
    if {[snd length] > 0} DrawOverAxis
#    if {$::tcl_platform(platform) == "unix"} {
#	if {$v(propflag) > 1} { pack propagate . 0 }
#    }
#    if {$dox && $doy} { incr v(propflag) }
}

proc SetMsg {msg args} {
    global v

    if {$args == ""} {
	set v(msg) $msg
	.bf.lab config -state disabled
    } elseif {$args == "mark"} {
	set v(msg) $msg
	set v(currline) -1
	.bf.lab config -state normal
    } else {
	set v(msg) $msg
	set v(currline) $args
	.bf.lab config -state normal
    }
    SetCursor ""
}

proc InputFromMsgLine key {
    global v labels
    
    if {$key == "BackSpace"} return
    if {$v(currline) >= 0} {
	set labels [lreplace $labels $v(currline) $v(currline) $v(msg)]
	Redraw quick
    } else {
	if {[scan $v(msg) {From: %s to: %s length: %s ( %f - %f , %f} t0 t1 t2 t3 t4 t5] == 6} {
	    if {$t0 != [lindex $v(fromto) 0]} {
		PutMarker m1 $t0 0 0
	    }
	    if {$t1 != [lindex $v(fromto) 1]} {
		set t2 [expr $t1-$t0]
		PutMarker m2 $t1 0 0
	    }
	    if {$t2 != [lindex $v(fromto) 2]} {
		set t1 [expr $t0+$t2]
		PutMarker m2 $t1 0 0
	    }
	    if {$t3 != [lindex $v(fromto) 3]} {
		set t0 [DTime2Time $t3]
		PutMarker m1 $t0 0 0
	    }
	    if {$t4 != [lindex $v(fromto) 4]} {
		set t1 [expr [DTime2Time $t4]-[DTime2Time $t3]]
		PutMarker m2 [DTime2Time $t4] 0 0
	    }
	    if {$t5 != [lindex $v(fromto) 5]} {
		set t1 [expr [DTime2Time $t3]+[DTime2Time $t5]]
		PutMarker m2 $t1 0 0
	    }
	    set t3 [format "%.3f" [Time2DTime $t0]]
	    set t4 [format "%.3f" [Time2DTime $t1]]
	    set t5 [format "%.3f" [expr $t4 - $t3]]
	    SetMsg [format "From: %9s to: %9s length: %9s\t(%.3f - %.3f, %.3f)"\
		    $t0 $t1 $t2 $t3 $t4 $t5] mark
	    set v(fromto) [list $t0 $t1 $t2 $t3 $t4 $t5]
	}
    }
}

proc PlayToCursor x {
    global c f v

    Stop snd
    if {[snd length] == 0} return
    set start [Marker2Sample m1]
    set s [Coord2Sample [$c canvasx $x]]
    if {$s < $start} {
	set end $start
	set start $s
    } else {
	set end $s
    }
    SetMsg "Playing range: $start $end"
    set v(s0) $start
    set v(s1) $end
    Play snd $start $end
    set v(pause) 0
    .of.c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    $c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    after 50 PutPlayMarker
}

proc PlayMark args {
    global c f v

    Stop snd
    if {[snd length] == 0} return
    set start [Marker2Sample m1]
    set end   [Marker2Sample m2]
    if {$start > [snd length]} return
    if {[llength $args] > 0} {
	set x [Coord2Sample [$c canvasx [lindex $args 0]]]
	if {$x < $start} {
	    set end $start
	    set start 0
	}
	if {$x > $end} {
	    set start $end
	    set end [snd length]
	}
    }
    if {$start == $end} {
	set start $end
	set end [snd length]
    }
    SetMsg "Playing range: $start $end"
    set v(s0) $start
    set v(s1) $end
    Play snd $start $end
    set v(pause) 0
    .of.c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    $c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    after 50 PutPlayMarker
}

proc PlayAll {} {
    global c v

    Stop snd
    SetMsg "Playing all samples"
    set v(s0) 0
    set v(s1) [snd length]
    Play snd
    set v(pause) 0
    .of.c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    $c create poly -1 -1 -1 -1 -1 -1 -fill red -tags playmark
    after 50 PutPlayMarker
}

proc Play {s {start 0} {end -1}} {
    global v

    if !$v(remote) {
	$s play -start $start -end $end
    } else {
	set sock [socket $v(ashost) $v(asport)]
	if {$end == -1} {
	    set end [snd length]
	}
	set v(rp_s) $s
	set v(rp_sock) $sock
	set end2 $end
	if {$end2 > [expr $start + 10000]} {
	    set end2 [expr $start + 10000]
	}
	set v(rp_next) $end2
	set v(rp_end) $end
	fconfigure $sock -translation binary -blocking 0
	puts -nonewline $sock play
	puts -nonewline $sock [$s data -fileformat au -start $start -end $end2]
	fileevent $sock writable PlayHandler
    }
}

proc PlayHandler {} {
    global v

    if {$v(rp_next) < $v(rp_end)} {
	set end2 $v(rp_end)
	if {$end2 > [expr $v(rp_next) + 10000]} {
	    set end2 [expr $v(rp_next) + 10000]
	}
	puts -nonewline $v(rp_sock) [$v(rp_s) data -fileformat raw -start $v(rp_next) -end $end2 -byteorder bigEndian]
	set v(rp_next) $end2
    } else {
	close $v(rp_sock)
    }
}

proc Stop s {
    global v

    if !$v(remote) {
	$s stop
    } else {
	catch {close $v(rp_sock)}
	catch {set sock [socket $v(ashost) $v(asport)]}
	if ![info exists sock] return
	fconfigure $sock -translation binary
	puts -nonewline $sock stop
	close $sock
    }
}

proc StopPlay {} {
    global c v

    after cancel PutPlayMarker
    Stop snd
    set v(pause) 0
    set v(s1) 0
    .of.c delete playmark
    $c delete playmark
    if $v(activerec) {
	after cancel UpdateRec
	Redraw
	event generate .cf.fc.c <Configure>
	MarkAll
	set v(activerec) 0
    }
}

proc PausePlay {} {
    global c v

    if $v(activerec) {
	snd pause
	return
    }
    set v(pause) [expr 1 - $v(pause)]
    if $v(pause) {
	after cancel PutPlayMarker
	set v(s0) [expr $v(s0) + int([audio elapsedTime] * $v(freq))]
	Stop snd
    } else {
	after 50 PutPlayMarker
	Play snd $v(s0) $v(s1)
    }
}

proc PutPlayMarker {} {
    global v c

    if $v(pause) return

    set time [expr [audio elapsedTime] + double($v(s0)) / $v(freq)]
    if {$time > [expr double($v(s1)) / $v(freq)] || ![audio active]} {
	.of.c delete playmark
	$c delete playmark
	return
    }
    SetMsg "Playing at [format "%.2f" $time]"
    set ox [expr $v(scrh) + $time * $v(opps) / 1000.0]
    set x [expr ($time - double($v(startsmp)) / $v(freq)) * $v(pps)]
    set y [expr $v(waveh) + $v(spegh) + 4]
    .of.c coords playmark $ox 22 [expr $ox-5] 30 [expr $ox+5] 30 
    $c coords playmark $x $y [expr $x-5] [expr $y+10] [expr $x+5] [expr $y+10]
    update idletasks
    after 50 PutPlayMarker
}

proc InfoStr arg {
    global f v labels

    set samps [snd length]
    set time  [snd length -units seconds]
    if {$arg == "path"} {
	set snd "$f(spath)$f(sndfile)"
	set lab "$f(lpath)$f(labfile)"
    } else {
	set snd $f(sndfile)
	set lab $f(labfile)
    }
    set info [format "Sample file: %s (%s)  %d samples %.2f seconds" $snd $v(smpfmt) $samps $time]
    if {$labels != {}} {
	set info "$info  Label file: $lab  ($v(labfmt))"
    }
    return $info
}

proc xsGetGeometry {} {
    scan [wm geometry .] "%dx%d+%d+%d" w h x y
    if {$::tcl_platform(platform) == "windows"} {
	return +$x+[expr $y+$h+40]
    } else {
	return +$x+[expr $y+$h+68]
    }
}

proc ToggleSpeg {} {
    global v

    if [audio active] return
    if $v(showspeg) {
        set v(spegh) $v(remspegh)
    } else {
        set v(remspegh) $v(spegh)
        set v(spegh) 0
    }
    Redraw
}

proc ToggleRecording {} {
    global v

    if $v(recording) {
	.tb.rec config -state normal
    } else {
	.tb.rec config -state disabled
    }

}

proc Record {} {
    global c v rec debug

    StopPlay
    set v(smpchanged) 1
    if [winfo exist .of] { pack forget .of }
    $c delete obj
    .of.c delete overwave
    set width [winfo width $c]
    $c xview moveto 0
    if {$v(waveh) > 0} {
	$c create waveform 0 0 -sound snd -height $v(waveh) -pixels $v(pps) \
		-width $width -tags [list obj recwave] -channel $v(vchan) \
		-debug $debug -fill red
    }
    if {$v(spegh) > 0} {
	$c create spectrogram 0 $v(waveh) -sound snd -height $v(spegh) \
		-pixels $v(pps) \
		-width $width -tags [list obj recwave] -channel $v(vchan) \
		-colormap $v($v(cmap)) -debug $debug
    }
    if {$v(linkfile)} {
	catch {file delete -force _xs[pid].wav}
	snd configure -file _xs[pid].wav
    }
    snd record
    set v(activerec) 1
    after 100 UpdateRec
}

proc UpdateRec {} {
    global c v

    if {$v(activerec) == 0} return
    set secs [expr int([snd length -units seconds])]
    set dec [format "%.2d" [expr int(100*([snd length -units seconds] - $secs))]]
    set time [clock format $secs -format "Length: %M:%S.$dec"]
#    if {$secs > 9} {
#	$c delete recwave rectext
#	$c create text [expr [lindex [$c xview] 0] * $v(width) + 60] 20 \
#		-fill red -text $time -tags [list obj rectext]
#	update
#    }
    SetMsg $time
    after 100 UpdateRec
}

proc MoveBoundary {x} {
    global c labels v

    set coords [$c coords current]
    set x [$c canvasx $x]
    if {$x < 0} { set x 0 }
    set i [string trim [lindex [$c gettags current] 0] b]
    if [string match [$c type current] text] return
    if {$i == "obj" || $i == "mark" || $i == "axis" || $i == ""} {
	return
    }

    set h [expr $i - 1]
    set j [expr $i + 1]

    if {$v(lastmoved) != $i} {
	set v(labchanged) 1
	SetUndo $labels
	set v(lastmoved) $i
    }

    $c raise current
    set px 0
    set nx $v(width)
    set pb [$c find withtag b$h]
    set nb [$c find withtag b$j]
    if {$pb != ""} { set px [lindex [$c coords $pb] 0]}
    if {$nb != ""} { set nx [lindex [$c coords $nb] 0]}

    if {$x <= $px} { set x [expr $px + 1] }
    if {$nx <= $x} { set x [expr $nx - 1] }

    $c coords current $x [lindex $coords 1] $x [lindex $coords 3]
    set rest ""

# Denna kod skulle kunna fixa så att lablar flyttas tillsammans med gränserna
#    set id [$c find withtag b$i]
#    $c coords lab$h $x [lindex $coords 1]
#    $c coords lab$j $x [lindex $coords 1]

    switch $v(labfmt) {
	TIMIT -
	HTK {
	    scan [lindex $labels $i] {%d %d %s %[^§]} start stop label rest
	    if {$j == [llength $labels]} { set length [expr $stop - $start] }
	    set start [Coord2Time $x]
	    if {$j == [llength $labels]} { set stop [expr $start + $length] }
	    set labels [lreplace $labels $i $i "$start\n$stop\n$label\n$rest"]
	    if {$h <= 0} return
	    while {[lindex [lindex $labels $h] 0] == [lindex [lindex $labels $h] 1]} {
		set hlabel [lindex [lindex $labels $h] 2]
		set hrest [lindex [lindex $labels $h] 3]
		set labels [lreplace $labels $h $h "$start\n$start\n$hlabel\n$hrest"]
		incr h -1
	    }
	    set rest ""
	    scan [lindex $labels $h] {%d %d %s %[^§]} start stop label rest
	    if {$v(labfmt) == "HTK"} {
		set stop [expr [Coord2Time $x]-(10000000/$v(freq))]
	    } else {
		set stop [Coord2Time $x]
	    }
	    set labels [lreplace $labels $h $h "$start\n$stop\n$label\n$rest"]
	}
	MIX {
	    scan [lindex $labels $i] {%d %s %[^§]} start label rest
	    set start [Coord2Time $x]
	    set labels [lreplace $labels $i $i "$start\n$label\n$rest"]
	}
	WAVES {
	    scan [lindex $labels $i] {%f %d %s %[^§]} end color label rest
	    set end [Coord2Time $x]
	    set labels [lreplace $labels $i $i "$end\n$color\n$label\n$rest"]
	}
    }
    SetMsg [Coord2Time $x]
}

proc SetLabelText {i label} {
    global labels v

    set rest ""
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    scan [lindex $labels $i] {%d %d %s %[^§]} start stop junk rest
	    set labels [lreplace $labels $i $i "$start\n$stop\n$label\n$rest"]
	}
	MIX {
	    scan [lindex $labels $i] {%d %s %[^§]} start junk rest
	    set labels [lreplace $labels $i $i "$start\n$label\n$rest"]
	}
	WAVES {
	    scan [lindex $labels $i] {%f %d %s %[^§]} end color junk rest
	    set labels [lreplace $labels $i $i "$end\n$color\n$label\n$rest"]
	}
    }
}

proc Undo {} {
    global c v labels undo

    if {[cbs length] != 0} {
	eval $v(undoc)
	set tmp $v(undoc)
	set v(undoc) $v(redoc)
	set v(redoc) $tmp
	DrawOverAxis
	Redraw
    } else {
	set tmp $labels
	set labels $undo
	set undo $tmp
	Redraw quick
    }
    SetMsg ""
}

proc SetUndo l {
    global undo

    set undo $l
    snack::menuEntryOn Edit Undo
    .tb.undo config -state normal
}

proc MarkAll {} {
    global v

    PutMarker m1 0 0 0
    PutMarker m2 [Coord2Time $v(width)] 0 0
}

proc ZeroXAdjust {} {
    global v
    
    foreach m {m1 m2} {
	set start [Marker2Sample $m]
	snd sample [expr $start-100] ;# to fill sample buffer with leftmost
	for {set i 0} {$i < 100} {incr i} {
	    set j [expr {$start + $i}]
	    if {$j >= [snd length]} break
	    if {$v(vchan) == 1} {
		set sample [lindex [snd sample $j] 1]
		set psample [lindex [snd sample [expr {$j-1}]] 1]
	    } else {
		set sample [lindex [snd sample $j] 0]
		set psample [lindex [snd sample [expr {$j-1}]] 0]
	    }
	    if {[expr {$sample*$psample}] < 0} break
	    set j [expr {$start - $i}]
	    if {$j < 0} break
	    if {$v(vchan) == 1} {
		set sample [lindex [snd sample $j] 1]
		set psample [lindex [snd sample [expr {$j-1}]] 1]
	    } else {
		set sample [lindex [snd sample $j] 0]
		set psample [lindex [snd sample [expr {$j-1}]] 0]
	    }
	    if {[expr {$sample*$psample}] < 0} break
	}
	if {$i < 100} {
	    PutMarker $m [Sample2Time $j] 0 0
	}

    }
    # Copied from PutMarker
    DrawZoom 1
    DrawSect
    set t1 [Marker2Time m1]
    set t2 [Marker2Time m2]
    set l  [expr $t2 - $t1]
    set tt1 [Time2DTime $t1]
    set tt2 [Time2DTime $t2]
    set ll  [expr $tt2 - $tt1]
    SetMsg [format "From: %9s to: %9s length: %9s\t(%.3f - %.3f, %.3f)"\
	    $t1 $t2 $l $tt1 $tt2 $ll] mark
    set v(fromto) [list $t1 $t2 $l $tt1 $tt2 $ll]
}

proc InsertLabel {x y} {
    global c v labels

    set v(labchanged) 1
    SetUndo $labels
    InsertLabelEntry [Coord2Time [$c canvasx $x]]

    $c delete bound text
    Redraw quick
}

proc InsertLabelEntry t {
    global labels v

    set i 0
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    foreach l $labels {
		if {([lindex $l 0] < $t) && ([lindex $l 1] > $t)} { break }
		incr i
	    }
	    if {[llength $labels] == $i} { incr i -1 }
	    if {$labels == ""} {
		set sto [DTime2Time [snd length -units seconds]]
		set labels [list "$t\n$sto\nx"]
	    } elseif {$t < [lindex [lindex $labels 0] 0]} {
		set sto [lindex [lindex $labels 0] 0]
		set labels [linsert $labels 0 "$t\n$sto\nx"]
	    } elseif {[llength $labels] == [expr $i+1]} {
		set sta1 [lindex [lindex $labels $i] 0]
		set sto1 $t
		set lab1 [lindex [lindex $labels $i] 2]
		set sta2 $t
		set sto2 [lindex [lindex $labels $i] 1]
		set lab2 x
		set labels [lreplace $labels $i $i "$sta1\n$sto1\n$lab1" "$sta2\n$sto2\n$lab2"]
            } else {
		SetMsg [lindex [lindex $labels $i] 2]
		set sta1 [lindex [lindex $labels $i] 0]
		set sto1 $t
		set lab1 [lindex [lindex $labels $i] 2]
		set sta2 $t
		set sto2 [lindex [lindex $labels [expr $i+1]] 0]
		set lab2 x
		set labels [lreplace $labels $i $i "$sta1\n$sto1\n$lab1" "$sta2\n$sto2\n$lab2"]
            }
	}
	MIX {
	    foreach l $labels {
		if {[lindex $l 0] > $t} { break }
		incr i
	    }
	    SetMsg [lindex [lindex $labels $i] 1]
	    set labels [linsert $labels $i "$t\nx"]
	}
	WAVES {
	    foreach l $labels {
		if {[lindex $l 0] > $t} { break }
		incr i
	    }
	    SetMsg [lindex [lindex $labels $i] 1]
	    set labels [linsert $labels $i "$t\n121\nx"]
	}
    }
}

proc DeleteLabel {x y} {
    global c v labels

    set v(labchanged) 1
    SetUndo $labels
    if {[string compare [lindex [$c gettags [$c find closest\
	    [$c canvasx $x] [$c canvasy $y]]] 2] text] == 0} {
	set i [lindex [$c gettags [$c find closest\
		[$c canvasx $x] [$c canvasy $y]]] 0]
	RemoveLabelEntry $i

	$c delete bound text
	Redraw quick
    }
}

proc RemoveLabelEntry i {
    global labels v

    switch $v(labfmt) {
	TIMIT -
	HTK {
	    set start [lindex [lindex $labels [expr $i-1]] 0]
	    set stop  [lindex [lindex $labels $i] 1]
	    set label [lindex [lindex $labels [expr $i-1]] 2]
	    set labels [lreplace $labels [expr $i-1] $i "$start\n$stop\n$label"]
	}
	WAVES -
	MIX {
	    set labels [lreplace $labels $i $i]
	}
    }
}

proc AlignLabel {x y} {
    global c v labels

    set v(labchanged) 1
    SetUndo $labels
    if {[string compare [lindex [$c gettags [$c find closest\
	    [$c canvasx $x] [$c canvasy $y]]] 2] text] == 0} {
	set i [lindex [$c gettags [$c find closest\
		[$c canvasx $x] [$c canvasy $y]]] 0]

	SetUndo $labels
	set start [Marker2Time m1]
	set end   [Marker2Time m2]
	set rest ""

	switch $v(labfmt) {
	    TIMIT -
	    HTK {
		scan [lindex $labels $i] {%d %d %s %[^§]} junk junk label rest
		set labels [lreplace $labels $i $i "$start\n$end\n$label\n$rest"]
		set rest ""
		set j [expr $i-1]
		if {$j >= 0} {
		    scan [lindex $labels $j] {%d %d %s %[^§]} st junk label rest
		    set labels [lreplace $labels $j $j "$st\n$start\n$label\n$rest"]
		}
		set rest ""
		set j [expr $i+1]
		if {$j < [llength $labels]} {
		    scan [lindex $labels $j] {%d %d %s %[^§]} junk st label rest
		    set labels [lreplace $labels $j $j "$end\n$st\n$label\n$rest"]
		}
	    }
	    MIX {
		scan [lindex $labels $i] {%d %s %[^§]} junk label rest
		set labels [lreplace $labels $i $i "$start\n$label\n$rest"]
		set rest ""
		set j [expr $i+1]
		catch {scan [lindex $labels $j] {%d %s %[^§]} junk label rest}
		catch {set labels [lreplace $labels $j $j "$end\n$label\n$rest"]}
	    }
	    WAVES {
		scan [lindex $labels $i] {%f %d %s %[^§]} junk color label rest
		set labels [lreplace $labels $i $i "$end\n$color\n$label\n$rest"]
		set rest ""
		set j [expr $i-1]
		if {$j >= 0} {
		    scan [lindex $labels $j] {%f %d %s %[^§]} junk color label rest
		    set labels [lreplace $labels $j $j "$start\n$color\n$label\n$rest"]
		}
	    }
	}

	$c delete bound text
	Redraw quick
    }
}

proc CropLabels {cstart cend} {
    global labels v

    set l {}
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    foreach row $labels {
		set rest ""
		scan $row {%d %d %s %[^§]} start stop label rest]
		if {$cend < $start} {
		} elseif {$cend > $start && $cend < $stop} {
		    set start [expr $start - $cstart]
		    set stop  [expr $cend - $cstart]
		    lappend l "$start\n$stop\n$label\n$rest"
		} elseif {$cstart > $start && $cstart < $stop} {
		    set start 0
		    set stop  [expr $stop  - $cstart]
		    lappend l "$start\n$stop\n$label\n$rest"
		} elseif {$cstart < $start} {
		    set start [expr $start - $cstart]
		    set stop  [expr $stop  - $cstart]
		    lappend l "$start\n$stop\n$label\n$rest"
		}
	    }
	}
	MIX {
	    foreach row $labels {
		set rest ""
		scan $row {%d %s %[^§]} start label rest
		if {$cend < $start} {
		} elseif {$cstart > $start} {
		    set l [list "0\n$label\n$rest"]
		} elseif {$cstart < $start} {
		    set start [expr $start - $cstart]
		    lappend l "$start\n$label\n$rest"
		}
	    }
	}
	WAVES {
	    set flag 0
	    foreach row $labels {
		set rest ""
		scan $row {%f %d %s %[^§]} end color label rest
		if {$cend < $end && $flag} {
		    set end [expr $cend - $cstart]
		    lappend l "$end\n$color\n$label\n$rest"
		    break
		}
		if {$cstart < $end} {
		    set end [expr $end - $cstart]
		    lappend l "$end\n$color\n$label\n$rest"
		    set flag 1
		}
	    }
	}
    }
    return $l
}

proc GetRightLabel {x y} {
    global c labels v

    set t [Coord2Time [$c canvasx $x]]
    set i 0
    set v(labchanged) 1
    SetUndo $labels
    set rest ""
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    foreach l $labels {
		if {$t < [lindex $l 0]} { break }
		if {([lindex $l 0] < $t) && ([lindex $l 1] > $t)} { break }
		incr i
	    }
	    if {[llength $labels] <= [expr $i+1]} return
	    if {$t < [lindex [lindex $labels 0] 0]} {
		set sto [lindex [lindex $labels 0] 1]
		set lab [lindex [lindex $labels 0] 2]
		set labels [lreplace $labels 0 0 "$t\n$sto\n$lab"]
	    } elseif {[llength $labels] == [expr $i-1]} {
		set sta1 [lindex [lindex $labels $i] 0]
		set sto1 $t
		set lab1 [lindex [lindex $labels $i] 2]
		set labels [lreplace $labels $i $i "$sta1\n$sto1\n$lab1"]
		SetMsg [lindex [lindex $labels $i] 2]		
            } else {
		set sta1 [lindex [lindex $labels $i] 0]
		set sto1 $t
		set lab1 [lindex [lindex $labels $i] 2]
		set sta2 $t
		set sto2 [lindex [lindex $labels [expr $i+1]] 1]
		set lab2 [lindex [lindex $labels [expr $i+1]] 2]
		set labels [lreplace $labels $i [expr $i+1] "$sta1\n$sto1\n$lab1" "$sta2\n$sto2\n$lab2"]
		SetMsg [lindex [lindex $labels $i] 2]
            }
        }
	MIX {
	    foreach l $labels {
		if {[lindex $l 0] > $t} { break }
		incr i
	    }
	    if {$i == [llength $labels]} return
	    scan [lindex $labels $i] {%d %s %[^§]} junk label rest
	    set labels [lreplace $labels $i $i "$t\n$label\n$rest"]
	    SetMsg [lindex [lindex $labels $i] 1]
	}
	WAVES {
	    foreach l $labels {
		if {([lindex $l 0] > $t)} { break }
		incr i
	    }
	    if {$i == [llength $labels]} return
	    scan [lindex $labels $i] {%f %d %s %[^§]} junk color label rest
	    set labels [lreplace $labels $i $i "$t\n$color\n$label\n$rest"]
	    SetMsg [lindex [lindex $labels $i] 1]
	}
    }
    $c delete bound text
    Redraw quick
}

proc PlayLabel {x y} {
    global c labels v

    set t [Coord2Time [$c canvasx $x]]
    set i 0
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    foreach l $labels {
		if {([lindex $l 0] < $t) && ([lindex $l 1] > $t)} { break }
		incr i
	    }
	    if {[llength $labels] == $i} { incr i -1 }
	}
	MIX {
	    foreach l $labels {
		if {[lindex $l 0] > $t} { break }
		incr i
	    }
	    incr i -1
	}
	WAVES {
	    foreach l $labels {
		if {[lindex $l 0] > $t} { break }
		incr i
	    }
	}
    }
    PlayNthLab $i
}

proc PlayNthLab n {
    global labels v
    
    switch $v(labfmt) {
	TIMIT -
	HTK {
	    set start [lindex [lindex $labels $n] 0]
	    set stop  [lindex [lindex $labels $n] 1]
	    Play snd [Time2Sample $start] [Time2Sample $stop]
	}
	MIX {
	    set start [lindex [lindex $labels $n] 0]
	    if {$n == -1} { set start 0 }
	    catch {set stop  [lindex [lindex $labels [expr $n + 1]] 0]}
	    if {$stop == ""} { set stop [Coord2Time $v(width)] }
	    Play snd [Time2Sample $start] [Time2Sample $stop]
	}
	WAVES {
	    set start [lindex [lindex $labels [expr $n - 1]] 0]
	    if {$start == ""} { set start 0 }
	    set stop  [lindex [lindex $labels $n] 0]
	    Play snd [Time2Sample $start] [Time2Sample $stop]
	}
    }
}

proc SetRaw {} {
    global v

    StopPlay
    set v(smpchanged) 1
    snd config -frequency $v(freq) -format $v(sfmt) -channels $v(chan)
    Redraw
    Reconf
}

proc Convert {format frequency channels} {
    global v c

    SetCursor watch    
    StopPlay
    set v(smpchanged) 1
    $c delete speg wave
    if {$frequency != ""} {
	snd convert -frequency $frequency
	set v(freq) [snd cget -frequency]
    }
    if {$format != ""} {
	snd convert -format $format
	set v(sfmt) [snd cget -format]
    }
    if {$channels != ""} {
	snd convert -channels $channels
	set v(chan) [snd cget -channels]
    }
    Redraw
}

proc Time2Sample t {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {int($t/(10000000/$v(freq)))}
	}
	TIMIT -
	MIX {
	    expr {int($t)}
	}
	WAVES {
	    expr {int($t*$v(freq))}
	}
    }
}

proc Sample2Time s {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {int($s*(10000000.0/$v(freq)))}
	}
	TIMIT -
	MIX {
	    set s
	}
	WAVES {
	    expr {double($s)/$v(freq)}
	}
    }
}

proc TimeRound t {
    global v

    switch $v(labfmt) {
	HTK -
	TIMIT -
	MIX {
	    expr {int($t)}
	}
	WAVES {
	    expr {$t}
	}
    }
}

proc Time2Coord t {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {(($t-10000000*(double($v(startsmp))/$v(freq)))/((10000000.0/$v(freq))*$v(freq)/$v(pps)))}
	}
	TIMIT -
	MIX {
	    expr {(($t - $v(startsmp)) / (double($v(freq))/$v(pps)))}
	}
	WAVES {
	    expr {(($t - (double($v(startsmp))/$v(freq)) )*$v(pps))}
	}
    }
}

proc Time2DTime t {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {($t/10000000.0)}
	}
	WAVES {
	    expr {$t}
	}
	TIMIT -
	MIX -
	default {
	    expr {double($t)/$v(freq)}
	}
    }
}

proc DTime2Time t {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {int($t*10000000.0)}
	}
	WAVES {
	    expr {$t}
	}
	TIMIT -
	MIX -
	default {
	    expr {int($t*$v(freq))}
	}
    }
}

proc Coord2Time x {
    global v

    switch $v(labfmt) {
	HTK {
	    expr {int(($x*$v(freq)/$v(pps)+$v(startsmp))*(10000000.0/$v(freq)))}
	}
	WAVES {
	    expr {double($x)/$v(pps)+double($v(startsmp))/$v(freq)}
	}
	TIMIT -
	MIX -
	default {
	    expr {int($v(startsmp)+$x*(double($v(freq))/$v(pps)))}
	}
    }
}

proc Coord2Sample x {
    global v

    expr {int($v(startsmp)+$x*double($v(freq))/$v(pps))}
}

proc BoundaryEnter x {
    global c _mb

    set _mb 1
    $c itemconfig current -fill red
    $c configure -cursor sb_h_double_arrow
}

proc BoundaryLeave x {
    global c v

    $c itemconfig current -fill $v(fg)
    $c configure -cursor {}
}

proc MarkerEnter x {
    global c

    $c itemconfig current -fill red
    $c configure -cursor sb_h_double_arrow
}

proc MarkerLeave x {
    global c v

    $c itemconfig current -fill $v(fg)
    $c configure -cursor {}
}

proc PutMarker {m x y f} {
    global c v _mx _mb

    if {$_mx == 0} return
    if {$y > [expr $v(waveh) + $v(spegh) + $v(timeh)]} {
	if {$_mb == 1 && $f == 1} {
	    MoveBoundary $x
	}
	return
    }
    if {$f == 1} {
	if {$x < 0 && [lindex [$c xview] 0] > 0} {
	    $c xview scroll -1 unit
	    update
	    SendXScroll
	}
	if {$x >= [winfo width $c]} {
	    $c xview scroll 1 unit
	    update
	    SendXScroll
	}

	set xc [$c canvasx $x]

	if {$xc < 0} { set xc 0 }
	if {$xc > $v(width)} { set xc $v(width) }

	set t [Coord2Time $xc]
    } else {
	set xc [Time2Coord $x]
	set t $x
    }

    $c itemconf $m -tags [list mark $t $m]
    $c coords $m $xc 0 $xc $v(toth)

    if {$m == "m1"} {
	set tm2 [Marker2Time m2]
	if {$t > $tm2} {
	    $c itemconf m2 -tags [list mark $tm2 m3]
	    $c itemconf m1 -tags [list mark $t m2]
	    $c itemconf m3 -tags [list mark [Marker2Time m3] m1]
	}
    } else {
	set tm1 [Marker2Time m1]
	if {$t < $tm1} {
	    $c itemconf m1 -tags [list mark $tm1 m3]
	    $c itemconf m2 -tags [list mark $t m1]
	    $c itemconf m3 -tags [list mark [Marker2Time m3] m2]
	}
    }

    if {$v(fillmark)} {
	$c coords mfill [Time2Coord [Marker2Time m1]] 0 \
		        [Time2Coord [Marker2Time m2]] $v(toth)
    }

    set ox1 [expr $v(scrh) + [Time2DTime [Marker2Time m1]] * $v(opps) / 1000.0]
    set ox2 [expr $v(scrh) + [Time2DTime [Marker2Time m2]] * $v(opps) / 1000.0]
    .of.c coords mark $ox1 2 $ox2 30

    if {$f == 1} {
	DrawZoom 1
	DrawSect
	set t1 [Marker2Time m1]
	set t2 [Marker2Time m2]
	set l  [expr $t2 - $t1]
	set tt1 [Time2DTime $t1]
	set tt2 [Time2DTime $t2]
	set ll  [expr $tt2 - $tt1]
	SetMsg [format "From: %9s to: %9s length: %9s\t(%.3f - %.3f, %.3f)"\
		$t1 $t2 $l $tt1 $tt2 $ll] mark
	set v(fromto) [list $t1 $t2 $l $tt1 $tt2 $ll]
    }

    foreach p $v(plugins) {
	namespace inscope $p Putmark $m
    }
    update
}

proc SendPutMarker {m x} {
    global c v

    set xc [$c canvasx $x]
    if {$v(mlink) == 1} {
	foreach prg [winfo interps] {
	    if [regexp .*xs.* $prg] {
		if {[winfo name .] != $prg} {
		    set t [Coord2Time $xc]
		    send $prg PutMarker $m $t 0 0
		}
	    }
	}
    }
}

proc Marker2Sample m {
    global c

    Time2Sample [lindex [$c gettags $m] 1]
}

proc Marker2Time m {
    global c

    lindex [$c gettags $m] 1
}

proc DrawCrossHairs {} {
    global c v

    if {$v(ch)} {
	$c delete ch1 ch2
	if {$::tcl_platform(platform) == "windows"} {
#	    $c create line 0 0 0 0 -width 2 -stipple gray50 -tags [list ch1]\
#		    -fill $v(gridcolor)
#	    $c create line 0 0 0 0 -width 2 -stipple gray50 -tags [list ch2]\
#		    -fill $v(gridcolor)
	    $c create line 0 0 0 0 -width 1 -tags [list ch1]\
		    -fill $v(gridcolor)
	    $c create line 0 0 0 0 -width 1 -tags [list ch2]\
		    -fill $v(gridcolor)
	} else {
	    $c create line 0 0 0 0 -width 1 -stipple gray50 -tags [list ch1]\
		    -fill $v(gridcolor)
	    $c create line 0 0 0 0 -width 1 -stipple gray50 -tags [list ch2]\
		    -fill $v(gridcolor)
	}
	$c lower ch1 m1
	$c lower ch2 m1
    } else {
	$c delete ch1 ch2
    }
}

proc PutCrossHairs {x y} {
    global c v

    set xc [$c canvasx $x]
    set yc [$c canvasy $y]
    set f 0.0
    catch { set f [expr $v(topfr) * ($v(spegh) - ($yc - $v(waveh))) / $v(spegh)]}
    if {$f < 0.0} { set f 0.0 }
    if {$f > 0.5*$v(freq)} { set f [expr 0.5*$v(freq)] }

    if {$v(ch)} {
	$c coords ch1 $xc 0 $xc $v(toth)
	$c coords ch2 0 $yc $v(width) $yc
	set s [Coord2Time $xc]
	set t [expr double($xc) / $v(pps)]

	SetMsg "time: $t\tsample: $s\tfrequency: $f"
    } else {
	$c coords ch1 -1 -1 -1 -1
	$c coords ch2 -1 -1 -1 -1
    }
    if [winfo exists .sect] { DrawSectMarks f $f }
}

proc OpenSectWindow {} {
    global s v

    catch {destroy .sect}
    toplevel .sect -width $s(sectwinw) -height $s(sectwinh)
    wm title .sect "Spectrum section plot"
    pack propagate .sect 0

    set s(ostart) ""

    pack [frame .sect.f] -side bottom -fill x
    label .sect.f.lab -width 1 -relief sunken -bd 1 -anchor w
    pack .sect.f.lab -side left -expand yes -fill x
    pack [button .sect.f.exitB -text Close -command {destroy .sect}] -side left
    pack [canvas .sect.c -closeenough 5 -cursor draft_small -bg $v(bg)] -fill both -expand true
    pack [frame .sect.f2]
    pack [button .sect.f2.lockB -text Lock -command {set s(ostart) $s(start);set s(oend) $s(end)}] -side left
    pack [button .sect.f2.printB -text Print -command {Print .sect.c $s(sectwinh)}] -side left

    update idletasks
    DrawSect

    bind .sect <Configure> DrawSect
    bind .sect.c <ButtonPress-1>  { set s(rx) %x; set s(ry) %y ;.sect.c coords relmark 0 0 0 0;.sect.c coords df -10 -10;.sect.c coords db -10 -10}
    bind .sect.c <ButtonRelease-1>  { set s(rx) -1 }
    bind .sect.c <Motion>  {DrawSectMarks %x %y}
    bind .sect.c <Leave>  {.sect.c coords sx -1 -1 -1 -1;.sect.c coords sy -1 -1 -1 -1}
}

proc DrawSect {} {
    global c s v debug

    if [winfo exists .sect] {
	set geom [lindex [split [wm geometry .sect] +] 0]
	set s(sectwinw) [lindex [split $geom x] 0]
	set s(sectwinh) [lindex [split $geom x] 1]
	set s(sectw) [expr [winfo width .sect.c] - 25]
	set s(secth) [expr [winfo height .sect.c] - 20]
	set s(sectcw) [winfo width .sect.c]
	set s(sectch) [winfo height .sect.c]

	set s(start) [Marker2Sample m1]
	set s(end)   [Marker2Sample m2]
	if {$s(start) == $s(end)} { set s(start) [expr $s(end) - 1]}
	.sect.c delete sect
	set s(winlen) [expr int($v(freq) / $s(anabw))]
	if {$s(ostart) != ""} {
	    .sect.c create section 25 0 -sound snd -height $s(secth)\
		    -width $s(sectw) -maxvalue 0 -minvalue -110 \
		    -start $s(ostart) -end $s(oend) -tags sect \
		    -fftlen $s(fftlen) \
		    -winlen $s(winlen) -channel $v(vchan) -fill red \
		    -topfr $v(topfr) 

	}
	.sect.c create section 25 0 -sound snd -height $s(secth) \
		-width $s(sectw) -maxval 0 -minval -110 \
		-start $s(start) -end $s(end) -tags sect -fftlen $s(fftlen) \
		-winlen $s(winlen) -channel $v(vchan) -frame 1 \
		-debug $debug -fill $v(fg) \
		-topfr $v(topfr)
	.sect.c create text -10 -10 -text df: -font $v(sfont) -tags df \
		-fill blue
	.sect.c create text -10 -10 -text "0 db" -font $v(sfont) -tags db \
		-fill red
	set pps [expr int($s(sectw)/($v(topfr)/1000.0) + .5)]
	snack::timeAxis .sect.c 25 $s(secth) $s(sectw) 20 $pps 1000 -tags sect \
		-fill $v(fg) -font $v(sfont)

	for {set db 10} {$db < 110} {incr db 10} {
	    .sect.c create text 0 [expr $db * $s(secth) / 110.0] -text -$db\
		    -tags sect -font $v(sfont) -anchor w -fill $v(fg)
	}
	.sect.c create text 2 2 -text dB -font $v(sfont) -tags sect -anchor nw\
		-fill $v(fg)
	.sect.c create text $s(sectw) $s(secth) -text kHz -font $v(sfont)\
		-tags sect -anchor nw -fill $v(fg)
    }
}

proc DrawSectMarks {x y} {
    global s v

    if {[.sect.c find withtag sm] == ""} {
	if {$::tcl_platform(platform) == "windows"} {
#	    .sect.c create line 0 0 0 $s(sectch) -width 2 -stipple gray50 -tags [list sx sm] -fill $v(fg)
#	    .sect.c create line 0 0 $s(sectcw) 0 -width 2 -stipple gray50 -tags [list sy sm] -fill $v(fg)
#	    .sect.c create line 0 0 0 0 -width 2 -stipple gray50 -tags [list relmark] -fill $v(fg)
	    .sect.c create line 0 0 0 $s(sectch) -width 1 -tags [list sx sm] -fill $v(fg)
	    .sect.c create line 0 0 $s(sectcw) 0 -width 1 -tags [list sy sm] -fill $v(fg)
	    .sect.c create line 0 0 0 0 -width 1 -tags [list relmark] -fill $v(fg)
	} else {
	    .sect.c create line 0 0 0 $s(sectch) -width 1 -stipple gray50 -tags [list sx sm] -fill $v(fg)
	    .sect.c create line 0 0 $s(sectcw) 0 -width 1 -stipple gray50 -tags [list sy sm] -fill $v(fg)
	    .sect.c create line 0 0 0 0 -width 1 -stipple gray50 -tags [list relmark relmarkux] -arrow both -fill $v(fg)
	}
    }

    if {$x != "f"} {
	set xc [.sect.c canvasx $x]
	set yc [.sect.c canvasx $y]
    } else {
	set xc [expr 25+int($s(sectw) * $y / $v(topfr))]
	set yc [lindex [.sect.c coords sy] 1]
    }
    .sect.c coords sx $xc 0 $xc $s(sectch)
    .sect.c coords sy 0 $yc $s(sectcw) $yc
    set f [expr int($v(topfr) * ($xc - 25) / $s(sectw))]
    if {$f < 0} { set f 0 }
    set db [format "%.1f" [expr -110.0 * $yc / $s(secth)]]

    if {$s(rx) != -1} {
	set rx [.sect.c canvasx $s(rx)]
	set ry [.sect.c canvasy $s(ry)]
	.sect.c coords relmark $rx $ry $xc $yc
	.sect.c coords df [expr $rx + ($xc-$rx)/2] $ry
	.sect.c coords db $rx [expr $ry + ($yc-$ry)/2]

	set df [expr abs(int($v(topfr) * ($rx - $xc)/ $s(sectw)))]
	.sect.c itemconf df -text "df: $df" 
	set ddb [format "%.1f" [expr 110.0 * ($ry - $yc) / $s(secth)]]
	.sect.c itemconf db -text "db: $ddb" 
    } else {
#	.sect.c coords relmark 0 0 0 0
#	.sect.c coords df -10 -10
#	.sect.c coords db -10 -10
    }

    .sect.f.lab config -text "Frequency: $f Hz, amplitude: $db dB"
}

proc OpenZoomWindow {} {
    global z v

    catch {destroy .zoom}
    catch {destroy .zmenu}
    toplevel .zoom -width $z(zoomwinw) -height $z(zoomwinh)
    wm title .zoom "Zoom view"
    pack propagate .zoom 0

    frame .zoom.f
    label .zoom.f.lab -text "Press right mouse button for menu" -width 1 -relief sunken -bd 1 -anchor w
    pack .zoom.f.lab -side left -expand yes -fill x
    pack [button .zoom.f.xzoomB -text X-zoom -command {DrawZoom 1}] -side left
    pack [button .zoom.f.yizoomB -text "Y-zoom in" -command {DrawZoom 2}] -side left
    pack [button .zoom.f.yozoomB -text "Y-zoom out" -command {DrawZoom .5}] -side left
    pack [button .zoom.f.exitB -text Close -command {destroy .zoom}] -side left
    pack .zoom.f -side bottom -fill x
    pack [canvas .zoom.c -closeenough 5 -bg $v(bg)] -fill both -expand true

    update idletasks
    DrawZoom 1

    menu .zmenu -tearoff false
    .zmenu add command -label "Play Range" -command PlayMark
    .zmenu add command -label "Mark Start" -command {PutZMarker zm1 $x}
    .zmenu add command -label "Mark End" -command {PutZMarker zm2 $x}
    bind .zoom.c <3> {set x %x; set y %y; catch {tk_popup .zmenu %X %Y 0}}

    bind .zoom <Configure> { DrawZoom 1 }
}

proc DrawZoom factor {
    global z v f

    if [winfo exists .zoom] {
	set geom [lindex [split [wm geometry .zoom] +] 0]
	set z(zoomwinw) [lindex [split $geom x] 0]
	set z(zoomwinh) [lindex [split $geom x] 1]
	set z(zoomwavw) [winfo width .zoom.c]
	set z(zoomwavh) [winfo height .zoom.c]
	set z(f) [expr $z(f) * $factor]

	set start [Marker2Sample m1]
	set end   [Marker2Sample m2]

	if {$start == $end} { set end [expr $start + 1]}
	set zoompps [expr double($z(zoomwavw)) * $v(freq) / ($end - $start)]

	.zoom.c delete zoomwave zm1 zm2
	if {$v(linkfile) && $f(sndfile) != ""} {
	    .zoom.c create waveform 0 [expr $z(zoomwavh)/2] -sound snd \
		    -height [expr int($z(zoomwavh) * $z(f))] \
		    -start $start -end $end	-channel $v(vchan) \
		    -pixels $zoompps -tags zoomwave -anchor w -fill $v(fg) \
		    -shapefile [file rootname $f(spath)$f(sndfile)].shape
	} else {
	    .zoom.c create waveform 0 [expr $z(zoomwavh)/2] -sound snd \
		    -height [expr int($z(zoomwavh) * $z(f))] \
		    -start $start -end $end	-channel $v(vchan) \
		    -pixels $zoompps -tags zoomwave -anchor w -fill $v(fg)
	}
	.zoom.c create line 1 0 1 $z(zoomwavh) -width 1 -tags zm1 -fill $v(fg)
	.zoom.c create line [expr $z(zoomwavw) - 1] 0 [expr $z(zoomwavw) - 1] $z(zoomwavh) -width 1 -tags zm2 -fill $v(fg)
	.zoom.c bind zm1 <B1-Motion> { PutZMarker zm1 %x }
	.zoom.c bind zm2 <B1-Motion> { PutZMarker zm2 %x }
	.zoom.c bind zm1 <ButtonPress-1> { set _mx 0 }
	.zoom.c bind zm2 <ButtonPress-1> { set _mx 0 }
	.zoom.c bind zm1 <ButtonRelease-1> { set _mx 0 }
	.zoom.c bind zm2 <ButtonRelease-1> { set _mx 0 }
	bind .zoom.c <ButtonPress-1>   { PutZMarker zm1 %x; set _mx 1 }
	bind .zoom.c <ButtonRelease-1> { PutZMarker zm2 %x; set _mx 1}
	set z(zoomt1) [Marker2Time m1]
	set z(zoomt2) [Marker2Time m2]
    }
}

proc PutZMarker {m x} {
    global z _mx

    if {$_mx == 0} return

    set xc [.zoom.c canvasx $x]
    if {$xc < 0} { set xc 0 }
    if {$xc > $z(zoomwavw)} { set xc $z(zoomwavw) }
    .zoom.c coords $m $xc 0 $xc $z(zoomwavh)

    set t [TimeRound [expr $z(zoomt1) + ($z(zoomt2) - $z(zoomt1)) * double($xc) / $z(zoomwavw)]]
    set n [Time2Sample $t]
    set s [snd sample $n]
    if {$m == "zm1"} {
	.zoom.f.lab config -text "Marker 1 at $n ($s)"
	PutMarker m1 $n 0 0
    } else {
	.zoom.f.lab config -text "Marker 2 at $n ($s)"
	PutMarker m2 $n 0 0
    }
}

proc OpenViewWindow {} {
    global c v

    catch {destroy .snack::view}
    toplevel .snack::view
    wm title .snack::view {View Control Panel}
    
    set contrast   [$c itemcget speg -contrast]
    set brightness [$c itemcget speg -brightness]

    set start [Coord2Sample [$c canvasx [expr [winfo width .cf.fc]/2 - 100]]]
    set end   [Coord2Sample [$c canvasx [expr [winfo width .cf.fc]/2 + 100]]]

    pack [canvas .snack::view.c -height 200 -width 200]
    .snack::view.c create spectrogram 0 0 -sound snd -fftlen $v(fftlen)\
	    -height 200 -width 200 -pixels $v(pps) \
	    -preemph $v(preemph) -topfr $v(topfr)\
	    -start $start -end $end -tags speg -contrast $contrast \
	    -brightness $brightness -gridtspacing $v(gridtspacing) \
	    -gridfspacing $v(gridfspacing) -channel $v(vchan) \
	    -colormap $v($v(cmap)) -gridcol $v(gridcolor) 

    pack [frame .snack::view.f1]
    pack [scale .snack::view.f1.b -label Brightness -variable brightness \
	    -orient horiz -command ".snack::view.c itemconf speg -brightness " \
	    -from -100 -to 100 -res 0.1 -length 200]
    
    pack [scale .snack::view.f1.c -label Contrast -variable contrast \
	    -orient horiz -command ".snack::view.c itemconf speg -contrast" \
	    -from -100 -to 100 -res 0.1 -length 200]
    pack [frame .snack::view.f2]
    pack [button .snack::view.f2.appB -text Apply -command { set v(contrast) $contrast; set v(brightness) $brightness; Redraw}] -side left
    pack [button .snack::view.f2.okB -text Ok -command {set v(contrast) $contrast; set v(brightness) $brightness; Redraw; destroy .snack::view}] -side left
    pack [button .snack::view.f2.canB -text Cancel -command {destroy .snack::view}] -side left
}

proc Version {} {
    global c v

    SetMsg "xs version $v(p_version), settings for $v(s_version)"
    catch {::http::geturl http://www.speech.kth.se/snack/xs.html\
	    -command VersionMore}
    set c .cf.fc.c
}

proc VersionMore token {
    global v

    set data [::http::data $token]
    regexp {version is ([0-9].[0-9])} $data junk version
    SetMsg "xs version $v(p_version), settings for $v(s_version), current download version is $version"
}

#
# Miscellaneous subroutines
#

proc Help url {
    global v lab_path
    
    if {$::tcl_platform(platform) == "windows"} {
	if {[string match $::tcl_platform(os) "Windows NT"]} {
	    exec $::env(COMSPEC) /c start $url &
	} {
	    exec start $url &
	}
    } else {
	if [catch {exec sh -c "netscape -remote 'openURL($url)' -raise"} res] {
	    if [string match *netscape* $res] {
		exec sh -c "netscape $url" &
	    }
	}
    }
}

proc NewWin {} {
    global f

    if {$::tcl_platform(platform) == "windows"} {
	exec [info nameofexecutable] $f(prog) &
    } else {
	exec $f(prog) -geometry [xsGetGeometry] &
    }
}

proc Reset {} {
    global v f s v_copy f_copy s_copy

    array set v $v_copy
    array set f $f_copy
    array set s $s_copy
}

proc Settings {} {
    global v c f s v_copy f_copy s_copy

    StopPlay
    set w .dim
    catch {destroy $w}
    toplevel $w
    wm title $w {Settings}

    set start [Coord2Sample [$c canvasx [expr [winfo width .cf.fc]/2 - 150]]]
    set end   [Coord2Sample [$c canvasx [expr [winfo width .cf.fc]/2 + 150]]]

    set v_copy [array get v]
    set f_copy [array get f]
    set s_copy [array get s]

    pack [frame $w.ll] -side left -anchor e
    pack [canvas $w.ll.c -height [expr $v(waveh)+$v(spegh)] -width 300 \
	    -highlightthickness 0]    

    pack [frame $w.l] -side left -anchor n -fill y
    pack [label $w.l.l1 -text Appearance:]

    pack [frame $w.l.f3]
    pack [label $w.l.f3.l -text "Time scale (pixels/second):" -width 25 -anchor w] -side left
    pack [entry $w.l.f3.e -textvar v(pps) -wi 6] -side left
    pack [scale $w.l.f3.s -variable v(pps) -orient horiz -from 1 -to 1000 -command "$w.ll.c itemconf both -width 300 -start $start -pixels " -showvalue no] -side left

    pack [frame $w.l.f1]
    pack [label $w.l.f1.l -text "Waveform height:" -width 25 -anchor w] -side left
    pack [entry $w.l.f1.e -textvar v(waveh) -wi 6] -side left
    pack [scale $w.l.f1.s -variable v(waveh) -orient horiz -from 0 -to 1000 -showvalue no -command {.dim.ll.c configure -height [expr $v(waveh) + $v(spegh)];.dim.ll.c coords speg 0 $v(waveh);.dim.ll.c itemconf wave -height }] -side left

    pack [frame $w.l.f2]
    pack [label $w.l.f2.l -text "Spectrogram height:" -width 25 -anchor w] -side left
    pack [entry $w.l.f2.e -textvar v(spegh) -wi 6] -side left
    pack [scale $w.l.f2.s -variable v(spegh) -orient horiz -from 0 -to 1000 -command {.dim.ll.c configure -height [expr $v(waveh) + $v(spegh)];.dim.ll.c itemconf speg -height } -showvalue no] -side left

    pack [frame $w.l.f20]
    pack [label $w.l.f20.l -text "Cut spectrogram at freq:" -width 25 -anchor w] -side left
    pack [entry $w.l.f20.e -textvar v(topfr) -wi 6] -side left
    pack [scale $w.l.f20.s -variable v(topfr) -orient horiz -from 0 -to [expr $v(freq)/2] -command "DrawSect;$w.ll.c itemconf speg -topfreq " -showvalue no] -side left

    pack [frame $w.l.f30]
    pack [label $w.l.f30.l -text "Brightness" -width 25 -anchor w] -side left
    pack [entry $w.l.f30.e -textvar v(brightness) -wi 6] -side left
    pack [scale $w.l.f30.b -variable v(brightness) -showvalue no \
	    -orient horiz -command "$w.ll.c itemconf speg -brightness " \
	    -from -100 -to 100 -res 0.1]

    pack [frame $w.l.f31]
    pack [label $w.l.f31.l -text "Contrast" -width 25 -anchor w] -side left
    pack [entry $w.l.f31.e -textvar v(contrast) -wi 6] -side left
    pack [scale $w.l.f31.c -variable v(contrast) -showvalue no\
	    -orient horiz -command "$w.ll.c itemconf speg -contrast" \
	    -from -100 -to 100 -res 0.1]

#    pack [frame $w.l.f21]
#    label $w.l.f21.l -text "Scroll area width:" -width 25 -anchor w
#    entry $w.l.f21.e -textvar v(scrw) -wi 6
#    pack $w.l.f21.l $w.l.f21.e -side left

    pack [frame $w.l.f4]
    label $w.l.f4.l -text "Milliseconds per tick:" -width 25 -anchor w
    entry $w.l.f4.e -textvar v(npt) -wi 6
    pack $w.l.f4.l $w.l.f4.e -side left

    pack [frame $w.l.f41]
    label $w.l.f41.l -text "Foreground color:" -width 25 -anchor w
    entry $w.l.f41.e -textvar v(fg) -wi 6
    pack $w.l.f41.l $w.l.f41.e -side left
    bind $w.l.f41.e <Key-Return> {.dim.ll.c itemconf wave -fill $v(fg)}

    pack [frame $w.l.f41b]
    label $w.l.f41b.l -text "Background color:" -width 25 -anchor w
    entry $w.l.f41b.e -textvar v(bg) -wi 6
    pack $w.l.f41b.l $w.l.f41b.e -side left
    bind $w.l.f41b.e <Key-Return> {$c config -bg $v(bg); .cf.fyc.yc config -bg $v(bg); catch {.zoom.c config -bg $v(bg)}; catch {.sect.c config -bg $v(bg)}}

    pack [frame $w.l.f42]
    label $w.l.f42.l -text "Grid frequency spacing (Hz):" -width 25 -anchor w
    entry $w.l.f42.e -textvar v(gridfspacing) -wi 6
    pack $w.l.f42.l $w.l.f42.e -side left
    bind $w.l.f42.e <Key-Return> {.dim.ll.c itemconf speg -gridf $v(gridfspacing)}

    pack [frame $w.l.f43]
    label $w.l.f43.l -text "Grid time spacing: (s)" -width 25 -anchor w
    entry $w.l.f43.e -textvar v(gridtspacing) -wi 6
    pack $w.l.f43.l $w.l.f43.e -side left
    bind $w.l.f43.e <Key-Return> {.dim.ll.c itemconf speg -gridt $v(gridtspacing)}

    pack [frame $w.l.f44]
    label $w.l.f44.l -text "Grid color:" -width 25 -anchor w
    entry $w.l.f44.e -textvar v(gridcolor) -wi 6
    pack $w.l.f44.l $w.l.f44.e -side left
    bind $w.l.f44.e <Key-Return> {DrawCrossHairs;.dim.ll.c itemconf speg -gridc $v(gridcolor)}

    pack [frame $w.l.f45]
    label $w.l.f45.l -text "Spectrogram color:" -width 25 -anchor w
    tk_optionMenu $w.l.f45.cm v(cmap) grey color1 color2
    $w.l.f45.cm.menu entryconfigure 0 -command {.dim.ll.c itemconf speg -col $v($v(cmap))}
    $w.l.f45.cm.menu entryconfigure 1 -command {.dim.ll.c itemconf speg -col $v($v(cmap))}
    $w.l.f45.cm.menu entryconfigure 2 -command {.dim.ll.c itemconf speg -col $v($v(cmap))}
    pack $w.l.f45.l $w.l.f45.cm -side left

    pack [label $w.l.l2 -text "Spectrogram analysis:"]

    pack [frame $w.l.f5]
    label $w.l.f5.l -text "FFT window length (points):" -width 25 -anchor w
    entry $w.l.f5.e -textvar v(fftlen) -wi 6
    pack $w.l.f5.l $w.l.f5.e -side left
    bind $w.l.f5.e <Key-Return> {.dim.ll.c itemconf speg -fftlen $v(fftlen)}

    pack [frame $w.l.f6]
    label $w.l.f6.l -text "Analysis bandwidth (Hz):" -width 25 -anchor w
    entry $w.l.f6.e -textvar v(anabw) -wi 6
    pack $w.l.f6.l $w.l.f6.e -side left
    bind $w.l.f6.e <Key-Return> {.dim.ll.c itemconf speg -winlen [expr int($v(freq) / $v(anabw))]}

    pack [frame $w.l.f7]
    label $w.l.f7.l -text "Preemphasis factor:" -width 25 -anchor w
    entry $w.l.f7.e -textvar v(preemph) -wi 6
    pack $w.l.f7.l $w.l.f7.e -side left
    bind $w.l.f7.e <Key-Return> {.dim.ll.c itemconf speg -preem $v(preemph)}

    pack [frame $w.r] -side left -anchor n -fill y -expand true

    pack [label $w.r.l3 -text "Spectrum section analysis:"] -pady 10

    pack [frame $w.r.f10]
    label $w.r.f10.l -text "FFT window length (points):" -width 25 -anchor w
    entry $w.r.f10.e -textvar s(fftlen) -wi 6
    pack $w.r.f10.l $w.r.f10.e -side left

    pack [frame $w.r.f11]
    label $w.r.f11.l -text "Analysis bandwidth (Hz):" -width 25 -anchor w
    entry $w.r.f11.e -textvar s(anabw) -wi 6
    pack $w.r.f11.l $w.r.f11.e -side left

#    pack [button $w.r.sectB -text Apply -command DrawSect] -pady 5
    bind $w.r.f10.e <Key-Return> DrawSect
    bind $w.r.f11.e <Key-Return> DrawSect

    pack [frame $w.r.f5]
    label $w.r.f5.l -text "Label font:" -width 11 -anchor w
    entry $w.r.f5.e -textvar v(font) -wi 20
    pack $w.r.f5.l $w.r.f5.e -side left

    pack [frame $w.r.f6]
    label $w.r.f6.l -text "Axes font:" -width 11 -anchor w
    entry $w.r.f6.e -textvar v(sfont) -wi 20
    pack $w.r.f6.l $w.r.f6.e -side left
    bind $w.r.f6.e <Key-Return> DrawSect

    pack [label $w.r.l4 -text "Raw/unknown file input:"] -pady 10
    pack [frame $w.r.f12]
    label $w.r.f12.l -text "Unknown file header size:" -width 25 -anchor w
    entry $w.r.f12.e -textvar f(skip) -wi 6
    pack $w.r.f12.l $w.r.f12.e -side left

    pack [frame $w.r.f9]
    label $w.r.f9.l -text "Byte order of sample data:" -width 25 -anchor w
    entry $w.r.f9.e -textvar f(byteOrder) -wi 12
    pack $w.r.f9.l $w.r.f9.e -side left

    pack [checkbutton $w.r.b5 -text "Use audio server at:" -var v(remote)] -pady 10
    pack [frame $w.r.f13]
    label $w.r.f13.l1 -text "Host" -width 4
    entry $w.r.f13.e1 -textvar v(ashost) -wi 20
    label $w.r.f13.l2 -text "Port" -width 4
    entry $w.r.f13.e2 -textvar v(asport) -wi 5
    pack $w.r.f13.l1 $w.r.f13.e1 $w.r.f13.l2 $w.r.f13.e2 -side left

#    pack [label $w.r.l5 -text "Browser command:"] -pady 5
#    pack [frame $w.r.f16]
#    entry $w.r.f16.e -textvar v(browser) -wi 30
#    pack $w.r.f16.e -side left

    pack [label $w.r.l6 -text "Initial path:"]
    pack [frame $w.r.f14]
    entry $w.r.f14.e -textvar f(ipath) -wi 30
    pack $w.r.f14.e -side left

    pack [label $w.r.l7 -text "Initial http:"]
    pack [frame $w.r.f15]
    entry $w.r.f15.e -textvar f(ihttp) -wi 30
    pack $w.r.f15.e -side left

    pack [frame $w.r.f] -anchor e -pady 5 -padx 5
    pack [button $w.r.f.okB -text OK -wi 6 -command {Redraw;destroy .dim}] -side right
    pack [button $w.r.f.appB -text Apply -wi 6 -command Redraw] -side right
    pack [button $w.r.f.exitB -text Cancel -command {Reset;DrawSect;Redraw;destroy .dim}] -side right
    update

    if {$v(linkfile) && $f(sndfile) != ""} {
	.dim.ll.c create waveform 0 0 -sound snd -height $v(waveh) -width 300 \
		-pixels $v(pps) -tags [list wave both] -start $start \
		-channel $v(vchan) -fill $v(fg) -frame yes -debug 0 \
		-shapefile [file rootname $f(spath)$f(sndfile)].shape
    } else {
	.dim.ll.c create waveform 0 0 -sound snd -height $v(waveh) -width 300 \
		-pixels $v(pps) -tags [list wave both] -start $start \
		-channel $v(vchan) -fill $v(fg) -frame yes -debug 0
    }
    if {$v(spegh) > 0} {
	.dim.ll.c create spectrogram 0 $v(waveh) -sound snd -fftlen $v(fftlen) \
		-height $v(spegh) -width 300 -pixels $v(pps) \
		-preemph $v(preemph) -topfr $v(topfr) \
		-start $start -tags [list speg both] \
		-contrast $v(contrast) \
		-brightness $v(brightness) -gridtspacing $v(gridtspacing) \
		-gridfspacing $v(gridfspacing) -channel $v(vchan) \
		-colormap $v($v(cmap)) -gridcol $v(gridcolor)
    }
}

proc Plugins {} {
    global v

    set w .plugins
    catch {destroy $w}
    toplevel $w
    wm title $w {Plug-ins}

    pack [ label $w.lPlugins -text "Installed plug-ins:"]
    pack [ frame $w.f] -fill both -expand true
    pack [ scrollbar $w.f.scroll -command "$w.f.list yview"] -side right -fill y
    listbox $w.f.list -yscroll "$w.f.scroll set" -setgrid 1 -height 6 -width 50
    pack $w.f.list -side left -expand true -fill both
    foreach e $v(pluginfiles) {
	$w.f.list insert end $e
    }

    pack [ label $w.lDesc -text Description:]
    pack [ frame $w.f2] -fill x
    pack [ text $w.f2.text -height 4 -wrap word] -fill x -expand true

    pack [ frame $w.f3]
    pack [ button $w.f3.b1 -text Load... -command "PluginsAdd $w"] -side left
    pack [ button $w.f3.b2 -text Unload -command "PluginsRemove $w"] -side left
    pack [ button $w.f3.b3 -text Close -command [list destroy $w]] -side left

    bind $w.f.list <ButtonRelease-1> {.plugins.f2.text delete 0.0 end;.plugins.f2.text insert end [namespace inscope [lindex $v(plugins) [.plugins.f.list curselection]] Describe]}
}

proc PluginsAdd w {
    global v

    set types {
	{{xs Plug-in Files} {.plg}}
	{{Tcl Files} {.tcl}}
	{{All Files}    *  }
    }
    set file [tk_getOpenFile -title "Select plug-in" -filetypes $types]
    if {$file == ""} return
    if {[source $file] == "fail"} return
    $w.f.list insert end $file
    set v(pluginfiles) [$w.f.list get 0 end]
}

proc PluginsRemove w {
    global v

    set i [$w.f.list curselection]
    namespace inscope [lindex $v(plugins) $i] Unload
    set v(plugins) [lreplace $v(plugins) $i $i]
    catch {$w.f.list delete $i}
    set v(pluginfiles) [$w.f.list get 0 end]
    $w.f2.text delete 0.0 end
}

proc Print {canvas h} {
    global v

    set w .print
    catch {destroy $w}
    toplevel $w
    wm title $w {Printer setup}

    set v(lastpage) [expr int(($v(width)+999)/1000)]
    set v(firstpage) 1

    frame $w.f1
    label $w.f1.l1 -text "Pages:"
    entry $w.f1.e1 -textvar v(firstpage) -width 3
    label $w.f1.l2 -text "to"
    entry $w.f1.e2 -textvar v(lastpage) -width 3
    pack $w.f1.l1 $w.f1.e1 $w.f1.l2 $w.f1.e2 -side left

    frame $w.f2
    label $w.f2.l1 -text "Print command:" -wi 16
    entry $w.f2.e1 -textvar v(printcmd)   -wi 40
    button $w.f2.b1 -text Print -command [list DoPrint print $canvas $h] -wi 8
    pack $w.f2.l1 $w.f2.e1 $w.f2.b1 -side left
    bind $w.f2.e1 <Key-Return> [list DoPrint print $canvas $h]

    frame $w.f3
    label $w.f3.l1 -text "Preview command:" -wi 16
    entry $w.f3.e1 -textvar v(gvcmd)        -wi 40
    button $w.f3.b1 -text Preview -command [list DoPrint preview $canvas $h] \
	    -wi 8
    pack $w.f3.l1 $w.f3.e1 $w.f3.b1 -side left
    bind $w.f3.e1 <Key-Return> [list DoPrint preview $canvas $h]

    frame $w.f4
    label $w.f4.l1 -text "Save to ps-file:" -wi 16
    entry $w.f4.e1 -textvar v(psfilet)       -wi 40
    button $w.f4.b1 -text Save -command [list DoPrint save $canvas $h] -wi 8
    pack $w.f4.l1 $w.f4.e1 $w.f4.b1 -side left
    bind $w.f4.e1 <Key-Return> [list DoPrint save $canvas $h]

    frame $w.f
    label $w.f.lab -text "" -width 1 -relief sunken -bd 1 -anchor w
    pack $w.f.lab -side left -expand yes -fill x
    button $w.f.exitB -text Close -command [list destroy $w]
    pack $w.f.exitB -side left
    pack $w.f1 $w.f2 $w.f3 $w.f4 $w.f -side top -fill x
}

proc DoPrint {type c canvh} {
    global v

    set n 0
    set pageno 0
    set x 0
    if {$c == ".sect.c"} {
	set w 1000
    } else {
	set w $v(width)
    }
    set title [InfoStr path]
    set time [clock format [clock seconds] -format "%a %b %d %T"]
    set width 1020
    set skip  1000

    if {$canvh == -1} {
	set canvh $v(toth)
    }
    
    $c delete ch1 ch2 sm
    $c itemconf relmarkux -stipple ""

    while {$w > 0} {
	incr pageno
	if {$pageno >= $v(firstpage)} {
	    if {$pageno > $v(lastpage)} break
	    $c create text [expr $x + 10] -10 -text "$title   Page: $pageno of $v(lastpage)   Printed: $time" -anchor w -tags decor
	    if {$c != ".sect.c"} {
		$c create line $x 0 $x $canvh -tags decor
		if {$w < $width} {
		    set ww [expr $x + $w]
		} else {
		    set ww [expr $x + $width]
		}
		$c create line $ww 0 $ww $canvh -tags decor
		snack::frequencyAxis $c $x [expr $v(waveh)-1] $v(yaxisw) \
			$v(spegh)\
			-topfrequency $v(topfr) -tags decor -fill $v(fg)
	    }
	    $c postscript -file _xspr$n.ps -colormode mono -rotate true -x $x -y -20 -width $width -height [expr $canvh + 20] -pagewidth 26c
	    
            switch $type {
		print {
		    regsub {\$FILE} $v(printcmd) _xspr$n.ps cmd
		}
		preview {
		    regsub {\$FILE} $v(gvcmd) _xspr$n.ps cmd
		}
		save {
		    regsub {\$FILE} $v(psfilecmd) _xspr$n.ps cmd
		    regsub {\$N} $v(psfilet) $n v(psfile)
		}
	    }
	    eval exec $cmd
	    file delete _xspr$n.ps
	    incr n
	    $c delete decor
	}
	incr x $skip
	incr w -$skip
    }
    if {$n == 1} {
	SetMsg "Printed 1 page"
    } else {
	SetMsg "Printed $n pages"
    }
    DrawCrossHairs
    $c itemconf relmarkux -stipple gray50
}

menu .popmenu -tearoff false
proc PopUpMenu {X Y x y} {
    global v

    .popmenu delete 0 end

    if {$y < [expr $v(waveh) + $v(spegh) + $v(timeh)]} {
	.popmenu add command -label "Play Range" -command [list PlayMark $x]
    } else {
	.popmenu add command -label "Play Label" -command [list PlayLabel $x $y]
    }
    .popmenu add command -label "Save Range" -command SaveMark
    .popmenu add command -label "Mark Start" -command "PutMarker m1 $x 0 1;SendPutMarker m1 $x"
    .popmenu add command -label "Mark End" -command "PutMarker m2 $x 0 1;SendPutMarker m2 $x"
    .popmenu add command -label "Zoom" -command OpenZoomWindow
    if {$y > [expr $v(waveh) + $v(spegh) + $v(timeh)]} {
	.popmenu add command -label "Insert Label" -command [list InsertLabel $x $y]
	.popmenu add command -label "Delete Label" -command [list DeleteLabel $x $y]
	.popmenu add command -label "Align Label" -command [list AlignLabel $x $y]
	.popmenu add command -label "Get Right Label" -command [list GetRightLabel $x $y]
    }
    catch {tk_popup .popmenu $X $Y 0}
}

proc SaveSettings {} {
    global v f s

    if [catch {open "~/.xsrc" w} out] {
	SetMsg $out
    } else {
	puts $out "set v(s_version) $v(p_version)"
	puts $out "set v(waveh) $v(waveh)"
	puts $out "set v(spegh) $v(spegh)"
#	puts $out "set v(scrw) $v(scrw)"
	puts $out "set v(pps) $v(pps)"
	puts $out "set v(npt) $v(npt)"
	puts $out "set v(fftlen) $v(fftlen)"
	puts $out "set v(winlen) $v(winlen)"
	puts $out "set v(anabw) $v(anabw)"
	puts $out "set v(preemph) $v(preemph)"
	puts $out "set v(ipa) $v(ipa)"
	puts $out "set v(autoload) $v(autoload)"
	puts $out "set v(ch) $v(ch)"
	puts $out "set v(iso) $v(iso)"
	puts $out "set v(slink) $v(slink)"
	puts $out "set v(mlink) $v(mlink)"
	puts $out "set v(printcmd) \{$v(printcmd)\}"
	puts $out "set v(gvcmd) \{$v(gvcmd)\}"
	puts $out "set v(pluginfiles) {$v(pluginfiles)}"
#	puts $out "set v(browser) \{$v(browser)\}"
	puts $out "set v(freq) $v(freq)"
	puts $out "set v(sfmt) $v(sfmt)"
	puts $out "set v(chan) $v(chan)"
#	puts $out "set v(offset) $v(offset)"
#	puts $out "set v(zerolabs) $v(zerolabs)"
	puts $out "set v(ipafmt) $v(ipafmt)"
	puts $out "set v(labalign) $v(labalign)"
	puts $out "set v(fg) $v(fg)"
	puts $out "set v(bg) $v(bg)"
	puts $out "set v(fillmark) $v(fillmark)"
	puts $out "set v(font) \{$v(font)\}"
	puts $out "set v(sfont) \{$v(sfont)\}"
	puts $out "set v(gridfspacing) $v(gridfspacing)"
	puts $out "set v(gridtspacing) $v(gridtspacing)"
	puts $out "set v(gridcolor) $v(gridcolor)"
	puts $out "set v(remote) \{$v(remote)\}"
	puts $out "set v(ashost) \{$v(ashost)\}"
	puts $out "set v(asport) \{$v(asport)\}"
	puts $out "set v(recording) \{$v(recording)\}"
	puts $out "set v(cmap) \{$v(cmap)\}"
	puts $out "set v(showspeg) \{$v(showspeg)\}"
	puts $out "set v(linkfile) \{$v(linkfile)\}"

	puts $out "set f(skip)  $f(skip)"
	puts $out "set f(byteOrder) $f(byteOrder)"
	puts $out "set f(ipath) $f(ipath)"
	puts $out "set f(ihttp) $f(ihttp)"
	puts $out "set f(guessraw) $f(guessraw)"

	puts $out "set s(fftlen) $s(fftlen)"
	puts $out "set s(winlen) $s(winlen)"
	puts $out "set s(anabw)  $s(anabw)"
	close $out
    }
}

proc SetCursor flag {
    foreach widget [winfo children .] {
	$widget config -cursor $flag
    }
    update idletasks
}

# Put custom procedures between the lines below
# Custom procs start here
# Custom procs end here

foreach plug [split $v(pluginfiles)] {
    source $plug
}

DrawCrossHairs
ToggleRecording
Link2File

if {$tcl_platform(platform) == "windows"} {
    update idletasks
    Redraw
}

proc GetStdin {} {
    global v pipevar

    append pipevar [read -nonewline stdin]
    if [eof stdin] { 
	fileevent stdin readable ""
	if {$pipevar != ""} {
	    snd data $pipevar
	    set v(freq) [snd cget -frequency]
	    set v(sfmt) [snd cget -format]
	    set v(chan) [snd cget -channels]
	    wm geometry . {}
	    Redraw
	    event generate .cf.fc.c <Configure>
	    MarkAll
	    PlayAll
	}
    }
}

if {$argv == "-"} {
    fconfigure stdin -translation binary -blocking 0
    if {$tcl_version > 8.0} {
	fconfigure stdin -encoding binary
    }
    fileevent stdin readable GetStdin
} elseif [llength $argv] {
    if {[llength $argv] > 1} { set v(autoload) 0 }
    foreach file $argv {
	OpenFiles $file
    }
} else {
    GetOpenFileName
}
