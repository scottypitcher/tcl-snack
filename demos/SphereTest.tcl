#!/usr/local/bin/tclsh

package require sound
package require snackSphere

sound s

set path ".."
set fileList [glob $path/nist/lib/data/ex*.wav] 

foreach file $fileList { 
    puts "Playing: $file"
    s config -file $file
    s play -block 1
}
