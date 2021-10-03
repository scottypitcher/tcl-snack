#!/bin/sh
# the next line restarts using wish \
exec tclsh8.3 "$0" "$@"

package require -exact snack 1.7
package require snacksphere

snack::sound s

set path ".."
set fileList [glob $path/nist/lib/data/ex*.wav] 

foreach file $fileList { 
    puts "Playing: $file"
    s config -file $file
    s play -block 1
}
