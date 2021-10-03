# 
# Copyright (C) 1997-99 Kare Sjolander <kare@speech.kth.se>
#
# This file is part of the Snack sound extension for Tcl/Tk.
# The latest version can be found at http://www.speech.kth.se/snack/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

package provide snack 1.7

namespace eval snack {
    namespace export gainBox get* add* menu* frequencyAxis timeAxis \
	    createIcons mixerDialog sound audio mixer debug

    #
    # Gain control dialog
    #

    proc gainBox flags {
	variable gainbox
	
	catch {destroy .snackGainBox}
	toplevel .snackGainBox
	wm title .snackGainBox {Gain Control Panel}
	
	if {[string match *p* $flags]} {
	    set gainbox(play) [snack::audio play_gain]
	    pack [scale .snackGainBox.s -label {Play volume} -orient horiz \
		    -variable snack::gainbox(play) \
		    -command {snack::audio play_gain} \
		    -length 200]
	}

	if {[snack::mixer inputs] != ""} {
	    if {[string match *r* $flags]} {
		set gainbox(rec)  [snack::audio record_gain]
		pack [scale .snackGainBox.s2 -label {Record gain} \
			-orient horiz \
			-variable snack::gainbox(rec) \
			-command {snack::audio record_gain} \
			-length 200]
	    }
	}
	pack [button .snackGainBox.exitB -text Close -command {destroy .snackGainBox}]
    }

    #
    # Snack mixer dialog
    #

    proc mixerDialog {} {
	set wi .snackMixerDialog
	catch {destroy $wi}
	toplevel $wi
	wm title $wi "Mixer"

#	pack [frame $wi.f0]
#	label $wi.f0.l -text "Mixer device:"
	
#	set outDevList [snack::mixer devices]
#	eval tk_optionMenu $wi.f0.om mixerDev $outDevList
#	pack $wi.f0.l $wi.f0.om -side left

	pack [frame $wi.f] -expand yes -fill both
	foreach line [snack::mixer lines] {
	    pack [frame $wi.f.g$line -bd 1 -relief solid] -side left \
		    -expand yes -fill both
	    pack [label $wi.f.g$line.l -text $line]
	    if {[snack::mixer channels $line] == "Mono"} {
		snack::mixer volume $line v(r$line)
	    } else {
		snack::mixer volume $line v(l$line) v(r$line)
		pack [scale $wi.f.g$line.e -from 100 -to 0 -show no \
			-var v(l$line)] -side left -expand yes -fill both
	    }
	    pack [scale $wi.f.g$line.s -from 100 -to 0 -show no \
		    -var v(r$line)] -expand yes -fill both
	}
	
	pack [frame $wi.f.f2] -side left
	
	if {[snack::mixer inputs] != ""} {
	    pack [label $wi.f.f2.li -text "Input jacks:"]
	    foreach jack [snack::mixer inputs] {
		snack::mixer input $jack [namespace current]::v(in$jack)
		pack [checkbutton $wi.f.f2.b$jack -text $jack \
			-variable [namespace current]::v(in$jack)] \
			-anchor w
	    }
	}
	if {[snack::mixer outputs] != ""} {
	    pack [label $wi.f.f2.lo -text "Output jacks:"]
	    foreach jack [snack::mixer outputs] {
		snack::mixer lines $jack [namespace current]::v(out$jack)
		pack [checkbutton $wi.f.f2.b$jack -text $jack \
			-variable [namespace current]::v(out$jack)] \
			-anchor w
	    }
	}
	pack [button $wi.b1 -text Close -com "destroy $wi"]
    }

    #
    # Snack filename dialog
    #

    proc getOpenFile {args} {
	upvar #0 __snack_args data
	
	set specs {
	    {-title       "" "" "Open file"}
	    {-initialdir  "" "" "."}
	    {-initialfile "" "" ""}
	    {-format      "" "" "none"}
	}
	
	tclParseConfigSpec __snack_args $specs "" $args
	
	if {$data(-format) == "none"} {
	    if {$data(-initialfile) != ""} {
		set data(-format) [ext2fmt [file extension $data(-initialfile)]]
	    } else {
		set data(-format) WAV
	    }
	}
	if {$data(-format) == ""} {
	    set data(-format) RAW
	}
	set data(-format) [string toupper $data(-format)]
	if {$data(-initialdir) == ""} {
	    set data(-initialdir) "."
	}
	return [tk_getOpenFile -title $data(-title) -filetypes [loadTypes $data(-format)] -defaultextension [fmt2ext $data(-format)] -initialdir $data(-initialdir) -initialfile $data(-initialfile)]
    }

    set loadTypes ""

    proc addLoadTypes {typelist fmtlist} {
	variable loadTypes
	variable filebox
	
	set loadTypes $typelist
	set i 9 ; # Needs updating when adding new formats
	foreach fmt $fmtlist { 
	    set filebox(l$fmt) $i
	    incr i
	}
    }

    proc loadTypes fmt {
	variable loadTypes
	variable filebox

	set l [concat {{{MS Wav Files} {.wav}} {{Smp Files} {.smp}} {{Snd Files} {.snd}} {{AU Files} {.au}} {{AIFF Files} {.aif}} {{AIFF Files} {.aiff}} {{Waves Files} {.sd}} {{MP3 Files} {.mp3}} {{CSL Files} {.nsp}}} $loadTypes {{{All Files} * }}]

	return [swapListElem $l $filebox(l$fmt)]
    }

    variable filebox
    set filebox(RAW) .raw
    set filebox(SMP) .smp
    set filebox(AU) .au
    set filebox(WAV) .wav
    set filebox(SD) .sd
    set filebox(SND) .snd
    set filebox(AIFF) .aif
    set filebox(MP3) .mp3
    set filebox(CSL) .nsp

    set filebox(lWAV) 0
    set filebox(lSMP) 1
    set filebox(lSND) 2
    set filebox(lAU)  3
    set filebox(lAIFF)  4
    # skip 2 because of aif and aiff
    set filebox(lSD)  6
    set filebox(lMP3)  7
    set filebox(lCSL)  8
    set filebox(lRAW) end
    # Do not forget to update indexes
    set filebox(sWAV) 0
    set filebox(sSMP) 1
    set filebox(sSND) 2
    set filebox(sAU)  3
    set filebox(sAIFF)  4
    # skip 2 because of aif and aiff
    set filebox(sCSL)  6
    set filebox(sRAW) end

    proc fmt2ext fmt {
	variable filebox

	return $filebox($fmt)
    }

    proc addExtTypes extlist {
	variable filebox

	foreach pair $extlist {
	    set filebox([lindex $pair 0]) [lindex $pair 1]
	}
    }

    proc getSaveFile args {
	upvar #0 __snack_args data

	set specs {
	    {-title       "" "" "Save file"}
	    {-initialdir  "" "" "."}
	    {-initialfile "" "" ""}
	    {-format      "" "" "none"}
	}

	tclParseConfigSpec __snack_args $specs "" $args

	if {$data(-format) == "none"} {
	    if {$data(-initialfile) != ""} {
		set data(-format) [ext2fmt [file extension $data(-initialfile)]]
	    } else {
		set data(-format) WAV
	    }
	}
	if {$data(-format) == ""} {
	    set data(-format) RAW
	}
	set data(-format) [string toupper $data(-format)]
	if {$data(-initialdir) == ""} {
	    set data(-initialdir) "."
	}
	return [tk_getSaveFile -title $data(-title) -filetypes [saveTypes $data(-format)] -defaultextension [fmt2ext $data(-format)] -initialdir $data(-initialdir) -initialfile $data(-initialfile)]
    }

    set saveTypes ""

    proc addSaveTypes {typelist fmtlist} {
	variable saveTypes
	variable filebox

	set saveTypes $typelist
	set j 7 ; # Needs updating when adding new formats
	foreach fmt $fmtlist {
	    set filebox(s$fmt) $j
	    incr j
	}
    }

    proc saveTypes fmt {
	variable saveTypes 
	variable filebox
	
	if {[info exists filebox(s$fmt)] == 0} {
	    set fmt RAW
	}
	set l [concat {{{MS Wav Files} {.wav}} {{Smp Files} {.smp}} {{Snd Files} {.snd}} {{AU Files} {.au}} {{AIFF Files} {.aif}} {{AIFF Files} {.aiff}} {{CSL Files} {.nsp}}} $saveTypes {{{All Files} * }}]

	return [swapListElem $l $filebox(s$fmt)]
    }

    proc swapListElem {l n} {
	set tmp [lindex $l $n]
	set l [lreplace $l $n $n]
	return [linsert $l 0 $tmp]
    }

    set filebox(.wav) WAV
    set filebox(.smp) SMP
    set filebox(.au) AU
    set filebox(.raw) RAW
    set filebox(.snd) SND
    set filebox(.sd) SD
    set filebox(.aif) AIFF
    set filebox(.aiff) AIFF
    set filebox(.mp3) MP3
    set filebox(.nsp) CSL
    set filebox() WAV

    proc ext2fmt ext {
	variable filebox

	return $filebox($ext)
    }

    #
    # Menus
    #

    proc menuInit { {m .menubar} } {
	variable menu

	menu $m
	[winfo parent $m] configure -menu $m
	set menu(menubar) $m
	set menu(uid) 0
    }

    proc menuPane {label {u 0}} {
	variable menu
	
	if [info exists menu(menu,$label)] {
	    error "Menu $label already defined"
	}
	if {$label == "Help"} {
	    set name $menu(menubar).help
	} else {
	    set name $menu(menubar).mb$menu(uid)
	}
	set m [menu $name -tearoff 1]
	$menu(menubar) add cascade -label $label -menu $name -underline $u
	incr menu(uid)
	set menu(menu,$label) $m
	return $m
    }

    proc menuDelete {menuName label} {
	variable menu

	set m [menuGet $menuName]
	if [catch {$m index $label} index] {
	    error "$label not in menu $menuName"
	}
	[menuGet $menuName] delete $index
    }

    proc menuGet menuName {
	variable menu
	if [catch {set menu(menu,$menuName)} m] {
	    return -code error "No such menu: $menuName"
	}
	return $m
    }

    proc menuCommand {menuName label command} {
	variable menu
	
	[menuGet $menuName] add command -label $label -command $command
    }

    proc menuCheck {menuName label var {command {}} } {
	variable menu

	[menuGet $menuName] add check -label $label -command $command \
		-variable $var
    }

    proc menuRadio {menuName label var {val {}} {command {}} } {
	variable menu
	
	if {[string length $val] == 0} {
	    set val $label
	}
	[menuGet $menuName] add radio -label $label -command $command \
		-value $val -variable $var
    }

    proc menuSeparator menuName {
	variable menu

	[menuGet $menuName] add separator
    }

    proc menuCascade {menuName label} {
	variable menu

	set m [menuGet $menuName]
	if [info exists menu(menu,$label)] {
	    error "Menu $label already defined"
	}
	set sub $m.sub$menu(uid)
	incr menu(uid)
	menu $sub -tearoff 0
	$m add cascade -label $label -menu $sub
	set menu(menu,$label) $sub
	return $sub
    }

    proc menuBind {what char menuName label} {
	variable menu

	set m [menuGet $menuName]
	if [catch {$m index $label} index] {
	    error "$label not in menu $menuName"
	}
	set command [$m entrycget $index -command]
	if {$::tcl_platform(platform) == "unix"} {
	    bind $what <Alt-$char> $command
	    $m entryconfigure $index -accelerator <Alt-$char>
	} else {
	    bind $what <Control-$char> $command
	    $m entryconfigure $index -accelerator <Control-$char>
	}
    }

    proc menuEntryOff {menuName label} {
	variable menu
	
	set m [menuGet $menuName]
	if [catch {$m index $label} index] {
	    error "$label not in menu $menuName"
	}
	$m entryconfigure $index -state disabled
    }

    proc menuEntryOn {menuName label} {
	variable menu
	
	set m [menuGet $menuName]
	if [catch {$m index $label} index] {
	    error "$label not in menu $menuName"
	}
	$m entryconfigure $index -state normal
    }

    #
    # Vertical frequency axis
    #

    proc frequencyAxis {canvas x y width height args} {
	set tags snack_y_axis
	set font {Helvetica 8}
	set topfr 8000
	set fill black
	set draw0 0

	for {set i 0} {$i < [expr [llength $args]]} {incr i 2} {
	    if {[lindex $args $i] == "-tags"} {
		set tags [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-font"} {
		set font [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-topfrequency"} {
		set topfr [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-fill"} {
		set fill [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-draw0"} {
		set draw0 [lindex $args [expr $i+1]]
	    } else {
		error "Unknown option: [lindex $args $i]"
	    }
	}

	if {$height <= 0} return
	set ticklist [list 10 20 50 100 200 500 1000 2000 5000 10000 20000 50000]
	set npt 10
	set dy [expr {double($height * $npt) / $topfr}]

	while {$dy < [font metrics $font -linespace]} {
	    foreach elem $ticklist {
		if {$elem <= $npt} {
		    continue
		}
		set npt $elem
		break
	    }
	    set dy [expr {double($height * $npt) / $topfr}]
	}

	if {$npt < 1000} { 
	    set hztext Hz
	} else {
	    set hztext kHz
	}

	if $draw0 {
	    set i0 0
	    set j0 0
	} else {
	    set i0 $dy
	    set j0 1
	}

	for {set i $i0; set j $j0} {$i < $height} {set i [expr {$i+$dy}]; incr j} {
	    set yc [expr {$height + $y - $i}]

	    if {$npt < 1000} { 
		set t [expr {$j * $npt}]
	    } else {
		set t [expr {$j * $npt / 1000}]
	    }
	    if {$yc > [expr {8 + $y}]} {
		if {[expr {$yc - [font metrics $font -ascent]}] > \
			[expr {$y + [font metrics $font -linespace]}] ||
		[font measure $font $hztext]  < \
			[expr {$width - 8 - [font measure $font $t]}]} {
		    $canvas create text [expr {$x + $width - 8}] [expr {$yc-2}]\
			    -text $t \
			    -font $font -anchor e -tags $tags -fill $fill
		}
		$canvas create line [expr {$x + $width - 5}] $yc \
			[expr {$x + $width}]\
			$yc -tags $tags -fill $fill
	    }
	}
	$canvas create text [expr {$x + 2}] [expr {$y + 1}] -text $hztext \
		-font $font -anchor nw -tags $tags -fill $fill

	return $npt
    }

    #
    # Horizontal time axis
    #

    proc timeAxis {canvas x y width height pps npt args} {
	set tags snack_t_axis
	set font {Helvetica 8}
	set starttime 0.0
	set fill black

	if {$pps <= 0} { return $npt }
	for {set i 0} {$i < [expr [llength $args]]} {incr i 2} {
	    if {[lindex $args $i] == "-tags"} {
		set tags [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-font"} {
		set font [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-starttime"} {
		set starttime [lindex $args [expr $i+1]]
	    } elseif {[lindex $args $i] == "-fill"} {
		set fill [lindex $args [expr $i+1]]
	    } else {
		error "Unknown option: [lindex $args $i]"
	    }
	}

	set ticklist [list 1 2 5 10 20 50 100 200 500 1000 2000 5000 10000 20000 50000 100000 200000 500000 1000000 2000000 5000000]

	if {$npt < 0} {
	    set npt 1
	}
	set dx [expr {$pps * $npt / 1000.0}]

# Compute the maximum width of a time label

        if {$width < 0.001} { set width 0.001 }
	if {$npt < 1000} { 
	    set l [expr {int(log10(1000*(double($width)/$pps+$starttime)))+1}]
	} else {
	    set l [expr {int(log10(double($width)/$pps+$starttime))+1}]
	}
	set tmp ""
	for {set i 0} {$i < $l} {incr i} { append tmp 0 }

# Compute the distance in pixels between tick marks

	while {$dx < [expr {4+[font measure $font $tmp]}]} {
	    foreach elem $ticklist {
		if {$elem <= $npt} {
		    continue
		}
		set npt $elem
		break
	    }
	    set dx [expr {$pps * $npt / 1000.0}]
	}

	set xo [expr {(int(0.5 + $starttime * 1000) % $npt) * $pps / 1000.0}]

	for {set i 0.0} {$i < $width} {set i [expr {$i + $dx}]} {
	    if {$npt < 1000} { 
		set t [expr {int(0.5 + 1000 * (($i - $xo) / $pps + $starttime))}]
	    } else {
		set t [expr {int(0.5 + (($i - $xo) / $pps + $starttime))}]
	    }
	    $canvas create text [expr {$x+$i+2-$xo}] [expr {$y+$height/2}] -text $t -font $font -anchor w -tags $tags -fill $fill
	    $canvas create line [expr {$x+$i-$xo}] $y [expr {$x+$i-$xo}] [expr {$y+$height}] -tags $tags -fill $fill
	}
	return $npt
    }

    #
    # Snack icons
    #

    variable icon
    set icon(open) R0lGODlhFAATAOMAAAAAAFeEAKj/AYQAV5o2AP8BqP9bAQBXhC8AhJmZmWZmZszMzAGo/1sB/////9zc3CH5BAEAAAsALAAAAAAUABMAQARFcMlJq13ANc03uGAoTp+kACWpAUjruum4nAqI3hdOZVtz/zoS6/WKyY7I4wlnPKIqgB7waet1VqHoiliE+riw3PSXlEUAADs=

    set icon(save) R0lGODlhFAATAOMAAAAAAAAAhAAA/wCEAACZmQD/AAD//4QAAISEAJmZmWZmZszMzP8AAP//AP///9zc3CH5BAEAAAsALAAAAAAUABMAQARBcMlJq5VACGDzvkAojiGocZWHUiopflcsL2p32lqu3+lJYrCZcCh0GVeTWi+Y5LGczY0RCtxZkVUXEEvzjbbEWQQAOw==

    set icon(print) R0lGODlhFAATAOMAAAAAAAAAhAAA/wCEAACZmQD/AAD//4QAAISEAJmZmWZmZszMzP8AAP//AP///9zc3CH5BAEAAAsALAAAAAAUABMAQARHcMlJq53A6b2BEIAFjGQZXlTGdZX3vTAInmiNqqtGY3Ev76bgCGQrGo8toS3DdIycNWZTupMITbPUtfQBznyz6sLl84iRlAgAOw==

    set icon(cut) R0lGODlhFAATAOMAAAAAAAAAhAAA/wCEAACZmQD/AAD//4QAAISEAJmZmWZmZszMzP8AAP//AP///9zc3CH5BAEAAAsALAAAAAAUABMAQAQ3cMlJq71LAYUvANPXVVsGjpImfiW6nK87aS8nS+x9gvvt/xgYzLUaEkVAI0r1ao1WMWSn1wNeIgA7

    set icon(copy) R0lGODlhFAATAOMAAAAAAAAAhAAA/wCEAACZmQD/AAD//4QAAISEAJmZmWZmZszMzP8AAP//AP///9zc3CH5BAEAAAsALAAAAAAUABMAQARFcMlJq5XAZSB0FqBwjSTmnF45ASzbbZojqrTJyqgMjDAXwzNSaAiqGY+UVsuYQRGDluap49RcpLjcNJqjaqEXbxdJLkUAADs=

    set icon(paste) R0lGODlhFAATAOMAAAAAAFeEAKj/AYQAV5o2AP8BqP9bAQBXhC8AhJmZmWZmZszMzAGo/1sB/////9zc3CH5BAEAAAsALAAAAAAUABMAQARTcMlJq11A6c01uFXjAGNJNpMCrKvEroqVcSJ5NjgK7tWsUr5PryNyGB04GdHE1PGe0OjrGcR8qkPPCwsk5nLCLu1oFCUnPk2RfHSqXms2cvetJyMAOw==

    set icon(undo) R0lGODlhFAATAOMAAAAAAAAAhAAA/wCEAACZmQD/AAD//4QAAISEAJmZmWZmZszMzP8AAP//AP///9zc3CH5BAEAAAsALAAAAAAUABMAQAQ7cMlJq6UKALmpvmCIaWQJZqXidWJboWr1XSgpszTu7nyv1IBYyCSBgWyWjHAUnE2cnBKyGDxNo72sKwIAOw==

    set icon(redo) R0lGODlhFAATAKEAAMzMzGZmZgAAAAAAACH5BAEAAAAALAAAAAAUABMAAAI4hI+py+0fhBQhPDCztCzSkzWS4nFJZCLTMqrGxgrJBistmKUHqmo3jvBMdC9Z73MBEZPMpvOpKAAAOw==

    set icon(gain) R0lGODlhFAATAOMAAAAAAFpaWjMzZjMAmZlmmapV/729vY+Pj5mZ/+/v78zM/wAAAAAAAAAAAAAAAAAAACH5BAEAAAUALAAAAAAUABMAAARnsMhJqwU4a32T/6AHdF8WjhUAAoa6kqwhtyW8uUlG4Tl2DqoJjzUcIAIeyZAmAiBwyhUNADQCAsHCUoVBKBTERLQ0RRiftLGoPGgDk1qpC+N2qXPM5lscL/lAAj5CIYQ5gShaN4oVEQA7

    set icon(zoom) R0lGODlhFAATAMIAAAAAAF9fXwAA/8zM/8zMzP///wAAAAAAACH5BAEAAAQALAAAAAAUABMAAAM/SLrc/jBKGYAFYapaes0U0I0VIIkjaUZo2q1Q68IP5r5UcFtgbL8YTOhS+mgWFcFAeCQEBMre8WlpLqrWrCYBADs=

    set icon(zoomIn) R0lGODlhFAATAMIAAMzMzF9fXwAAAP///wAA/8zM/wAAAAAAACH5BAEAAAAALAAAAAAUABMAAANBCLrc/jBKGYQVYao6es2U0FlDJUjimFbocF1u+5JnhKldHAUB7mKom+oTupiImo2AUAAmAQECE/SMWp6LK3arSQAAOw==

    set icon(zoomOut) R0lGODlhFAATAMIAAMzMzF9fXwAAAP///wAA/8zM/wAAAAAAACH5BAEAAAAALAAAAAAUABMAAANCCLrc/jBKGYQVYao6es2U0I2VIIkjaUbidQ0r1LrtGaRj/AQ3boEyTA6DCV1KH82iQigUlYAAoQlUSi3QBTbL1SQAADs=

    proc createIcons {} {
	variable icon

	image create photo snackOpen  -data $icon(open)
	image create photo snackSave  -data $icon(save)
	image create photo snackPrint -data $icon(print)
	image create photo snackCut   -data $icon(cut)
	image create photo snackCopy  -data $icon(copy)
	image create photo snackPaste -data $icon(paste)
	image create photo snackUndo  -data $icon(undo)
	image create photo snackRedo  -data $icon(redo)
	image create photo snackGain  -data $icon(gain)
	image create photo snackZoom  -data $icon(zoom)
	image create photo snackZoomIn -data $icon(zoomIn)
	image create photo snackZoomOut -data $icon(zoomOut)
    }

    #
    # Support routines for shape files
    #

    proc deleteInvalidShapeFile {fileName} {
	if {$fileName == ""} return
	if ![file exists $fileName] return
	set shapeName ""
	if [file exists [file rootname $fileName].shape] {
	    set shapeName [file rootname $fileName].shape
	}
	if [file exists [file rootname [file tail $fileName]].shape] {
	    set shapeName [file rootname [file tail $fileName]].shape
	}
	if {$shapeName != ""} {
	    set fileTime [file mtime $fileName]
	    set shapeTime [file mtime $shapeName]
	    if {$fileTime > $shapeTime} {

		# Delete shape file if older than sound file

		file delete -force $shapeName
	    } else {
		set s [snack::sound]
		$s config -file $fileName
		set soundSize [expr {200*[$s length -units seconds]}]
		set shapeSize [file size $shapeName]
		if {[expr {$soundSize*0.95}] > $shapeSize || \
			[expr {$soundSize*1.05}] < $shapeSize} {

		    # Delete shape file with incorrect size

		    file delete -force $shapeName
		}
		$s destroy
	    }
	}
    }

    proc makeShapeFileDeleteable {fileName} {
	if {$::tcl_platform(platform) == "unix"} {
	    if [file exists [file rootname $fileName].shape] {
		set shapeName [file rootname $fileName].shape
		catch {file attributes $shapeName -permissions 0777}
	    }
	    if [file exists [file rootname [file tail $fileName]].shape] {
		set shapeName [file rootname [file tail $fileName]].shape
		catch {file attributes $shapeName -permissions 0777}
	    }
	}
    }

    #
    # Snack default progress callback
    #

    proc progressCallback {message fraction} {
	set w .snackProgressDialog

	if {$fraction == 1.0} {

	    # Task is finished close dialog

	    destroy $w
	    return
	}
	if {![winfo exists $w]} {

	    # Open progress dialog if not currently shown

	    toplevel $w
	    pack [label $w.l]
	    pack [canvas $w.c -width 200 -height 20 -relief sunken \
		    -borderwidth 2]
	    $w.c create rect 0 0 0 20 -fill black -tags bar
	    pack [button $w.b -text Stop -command "destroy $w.b"]
	    wm title $w "Please wait..."
	    wm transient $w .
	    wm withdraw $w
	    set x [expr {[winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
		    - [winfo vrootx [winfo parent $w]]}]
	    set y [expr {[winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
		    - [winfo vrooty [winfo parent $w]]}]
	    wm geom $w +$x+$y
	    wm deiconify $w
	    update idletasks
	} elseif {![winfo exists $w.b]} {

	    # User hit Stop button, close dialog

	    destroy $w
	    return -code error
	}
	switch -- $message {
	    "Convert frequency" {
		set message "Converting frequency..."
	    }
	    "Convert format" {
		set message "Converting sample encoding format..."
	    }
	    "Convert channels" {
		set message "Converting number of channels..."
	    }
	    "Computing pitch" {
		set message "Computing pitch..."
	    }
	    "Read sound" {
		set message "Reading sound..."
	    }
	    "Write sound" {
		set message "Writing sound..."
	    }
	    "Computing waveform" {
		set message "Waveform is being precomputed and\
			stored on disk..."
	    }
	}
	$w.l configure -text $message
	$w.c coords bar 0 0 [expr {$fraction * 200}] 20
	update
    }
}
