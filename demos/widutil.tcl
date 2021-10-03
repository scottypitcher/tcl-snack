proc Run {script {demoFlag 0}} {
    set i [interp create]
    load {} Tk $i
    $i eval rename exit dontexit
    interp alias $i exit {} interp delete $i
    if {$demoFlag != 0} {
	$i eval set demoFlag $demoFlag
    }
    $i eval wm title . $script
    $i eval source $script
}

proc Browse file {
    set w .browse
    catch {destroy $w}
    toplevel $w
    wm title $w "View source: $file"

    pack [ button $w.b -text Close -command "destroy $w"] -side bottom
    pack [ frame $w.f] -fill both -expand yes
    text $w.f.t -width 60 -height 20 -setgrid true -wrap none
    $w.f.t config -xscrollcommand [list $w.f.xscroll set] -yscrollcommand [list $w.f.yscroll set]
    scrollbar $w.f.xscroll -orient horizontal -command [list $w.f.t xview]
    scrollbar $w.f.yscroll -orient vertical -command [list $w.f.t yview]
    pack $w.f.xscroll -side bottom -fill x
    pack $w.f.yscroll -side right -fill y
    pack $w.f.t -side left -fill both -expand yes

    if [catch {open $file} in] {
	set text $in
    } else {
	catch {set text [read $in]}
    }
    $w.f.t insert 1.0 $text
}

proc BindDrag c {
    bind $c <1> "initDrag $c %x %y"
    bind $c <B1-Motion> "Drag $c %x %y"
}
proc initDrag {c x y} {
    global ox oy

    set ox [$c canvasx $x]
    set oy [$c canvasy $y]
}

proc Drag {c x y} {
    global ox oy

    set x [$c canvasx $x]
    set y [$c canvasy $y]
    $c move current [expr $x - $ox] [expr $y - $oy]
    set ox $x
    set oy $y
}
