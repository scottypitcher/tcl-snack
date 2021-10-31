#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

script=" \
    puts {Test script ...} ; \
    package require tcltest ; \
    ::tcltest::configure -verbose {start pass error } ; \
    ::tcltest::testsDirectory [file normalize tests] ; \
    ::tcltest::workingDirectory [file normalize {demos/tcl}] ; \
    namespace import ::tcltest::* ; \
    if {[catch {source ../../tests/all.tcl} result]} { puts \"error=\$result\" ; puts \"errorInfo=\$::errorInfo\" } ; \
"
echo $script | $TCLSH_BIN


