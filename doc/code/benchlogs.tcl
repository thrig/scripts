#!/usr/bin/env expect
set logfile  scratch
set buffhows [list none full]
set lengths  [list 64 4096]
set logbytes 2097152
set trials   500
proc writelogs {logfile buffhow logbytes length trials} {
    set statfh  [open stats.$length.$buffhow w]
    set logline [string repeat a [expr $length - 1]]
    while {$trials} {
        set  logfh [open $logfile w]
        chan configure $logfh -buffering $buffhow
        set iterations [expr int($logbytes / $length)]
        set timer [time {
            while {$iterations} {
                puts $logfh $logline
                incr iterations -1
            }
            if {$buffhow eq "full"} {
                chan flush $logfh
            }
        }]
        puts  $statfh [lindex [split $timer " "] 0]
        close $logfh
        incr  trials -1
    }
}
foreach buffhow $buffhows {
    foreach loglength $lengths {
        writelogs $logfile $buffhow $logbytes $loglength $trials
    }
}
