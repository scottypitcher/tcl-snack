#!/bin/sh
# the next line restarts using wish \
exec wish8.3 "$0" "$@"

# This utility creates stand-alone executables from Snack Tcl scripts
# It needs the freewrap tool which can be downloaded from
# http://freewrap.sourceforge.net/
# Edit this file to set the freewrap variable below according to
# your freewrap install path.


pack [label .l1 -text "This utility creates\nstand-alone executables"]
update

# Get the Snack package's directory

package require -exact snack 2.2
set tmp [package ifneeded snack [package provide snack]]
set tmp [lindex [lindex [split $tmp ";"] 0] end]
set snackdir [file dirname $tmp]

switch -glob [string tolower $tcl_platform(platform)] {
 windows {
# Customize freewrap install dir here
# freewrap.exe is included in the Windows binary distribution
  set freewrap [file join [pwd] freewrap.exe]
  set tmpdir C:/temp
  regsub -all {\\} $tmpdir / tmpdir
  set wrapdir [file join $tmpdir wrap]
  set appextension .exe
 }
 unix {
# Customize freewrap install dir here
# freewrap executable for Linux-i386 is included in the source distribution
  set freewrap [file join [pwd] freewrap]
  set tmpdir /tmp
  set wrapdir [file join $tmpdir wrap]
  set appextension ""
 }
 default {
  error "unknown os $tcl_platform(os)"
 }
}
if {[info exists argv] == 0} { set argv "" }
set mainprog [file rootname [lindex $argv 0]]
if {$mainprog == ""} {
 set mainprog [file rootname [lindex [file split \
	 [tk_getOpenFile -filetypes {{{Tcl scripts} {.tcl}}}]] end]]
  if {$mainprog == ""} return
}

if {[file executable $freewrap] == 0} {
    tk_messageBox -message "This utility requires the freewrap program which is available at http://freewrap.sourceforge.net/. Edit wrap.tcl in order to specify its install path."
 exit
}

# Clean-up previous work directory

file delete -force $wrapdir

# Copy Snack's files

foreach {dir list} [list $wrapdir/snack \
	[list $snackdir/libsnack[info sharedlibextension] \
	$snackdir/snack.tcl $snackdir/pkgIndex.tcl ]] {
 file mkdir $dir
 foreach file $list {
  file copy $file $dir
 }
}

# Copy standard extension packages if they are needed

set f0 [open ${mainprog}.tcl r]
while {[eof $f0] == 0} {
  set line [gets $f0]
  if {[string match {*package require*snackogg*} $line]} {
    file copy $snackdir/libsnackogg[info sharedlibextension] $dir
  }
  if {[string match {*package require*snacksphere*} $line]} {
    file copy $snackdir/libsnacksphere[info sharedlibextension] $dir
  }
}
close $f0

# copy script files to the wrap directory, and insert magic code

set files ${mainprog}.tcl
set files [concat $files [lrange $argv 1 end]]

foreach fn $files {
  if {[string match *.tcl $fn]} {
    set f1 [open $fn r]
    set f2 [open $wrapdir/$fn w]

    set search 1
    while {$search == 1 && [eof $f1] == 0} {
      set line [gets $f1]
      if [string match {package require*snack*} $line] {
	puts $f2 "set auto_path \"\[file join $tmpdir wrap snack\] \$auto_path\""
	puts $f2 "rename load _load"
	puts $f2 "proc load {fname args} {"
	puts $f2 { set f [open $fname]}
	puts $f2 { fconfigure $f -encoding binary -translation binary}
	puts $f2 { set data [read $f]}
	puts $f2 { close $f}

	puts $f2 " switch -glob \[string tolower \$::tcl_platform(platform)\] {"
	puts $f2 "  unix {set tmpdir \"/tmp\"}"
	puts $f2 "  win* {"
	puts $f2 "    if {\[info exists ::env(TEMP)] && \$::env(TEMP) != \"\"} {"
	puts $f2 {      set tmpdir $::env(TEMP)}
	puts $f2 "    } elseif {\[info exists ::env(TMP)\] && \$::env(TMP) != \"\"} {"
	puts $f2 {      set tmpdir $::env(TMP)}
	puts $f2 "    } else {"
	puts $f2 {      set tmpdir ""}
	puts $f2 "    }"
	puts $f2 "  }"
	puts $f2 "  macintosh {"
	puts $f2 "    set tmpdir ."
	puts $f2 "  }"
	puts $f2 " }"


	puts $f2 " set fname2 \[file join \$tmpdir \[file rootname \[file tail \$fname\]\].\[pid\]\]"
	puts $f2 { set f [open $fname2 w]}
	puts $f2 { fconfigure $f -encoding binary -translation binary}
	puts $f2 { puts -nonewline $f $data}
	puts $f2 { close $f}
	puts $f2 { eval _load $fname2 $args}
	puts $f2 " foreach f \[glob -nocomplain \[file join \$tmpdir libsnack*\]\] \{"
	puts $f2 "  catch \{file delete -force \$f\}"
	puts $f2 " \}"
	puts $f2 "}"
	set search 0
      }
      puts $f2 $line
    }
    
    # copy the rest
    puts $f2 [read $f1]
    close $f1
    close $f2
  } else {
    if {$fn != ""} {
      file copy $fn [file join $wrapdir $fn]
    }
  }
}

set appdir [pwd]
cd $wrapdir

set wrapfiles ""
foreach item [glob -nocomplain * */* */*/*] {
 if [file isdirectory $item] continue
 lappend wrapfiles [file join $wrapdir $item]
}

# Do the actual wrapping

eval exec $freewrap ${mainprog}.tcl $wrapfiles
file copy -force $wrapdir/${mainprog}$appextension $appdir

catch {exec chmod -R 777 $wrapdir}
catch {tk_messageBox -message "Created: ${mainprog}$appextension"}
cd $appdir
exit
