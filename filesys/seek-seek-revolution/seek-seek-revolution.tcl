#!/usr/bin/env tclsh8.6

set   fh [open revolution w+] 
puts $fh "echo say what"
puts $fh "sleep 1"
seek $fh 0 start

exec sh <@ $fh &

while 1 { seek $fh 0 start; after 10 }
