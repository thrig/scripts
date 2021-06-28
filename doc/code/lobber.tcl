#!/usr/bin/env expect
for {set i 0} {$i < 25} {incr i} {
    set pid [fork]
    if {$pid == -1} { puts stderr "fork failed"; exit 1 }
    if {$pid != 0} { continue }

    set fh [open "| socat - UNIX-CONNECT:thesocket" w]
    chan configure $fh -buffering none

    set char    [format %c [expr 97 + $i]]
    set teststr [string repeat $char 9999]
    for {set rep 100} {$rep > 0} {incr rep -1} {
        puts -nonewline $fh $teststr
    }
    exit 0
}
wait -i -1
