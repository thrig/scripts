# FIXME for testing via MacPorts installed named
set nsupdate_cmd /opt/local/bin/nsupdate
# NOTE "-t 0" per nsupdate(1) says it should "disable the timeout" but
# instead with bind 9.12 a "dns_request_createvia3: out of range"
# message appears before nsupdate fails
set nsupdate_args {-k /opt/local/etc/nsuk.key -t 60}

# FIXME this used to be example.net but that is variously problematical;
# per RFC 6761 going instead with a .test subdomain which doubtless will
# be problematical in new and exciting ways
set domain dnstw.test.

# this is disabled so unit tests can tell the difference between server
# not being set and when -s is given at the command line though likely
# should be set if you want to ensure that the updates go to a
# particular server and do not wander out somewhere to the Internet
#set server 127.0.0.1

set default_mx_priority 10

set TTL_MIN 60
set TTL_MAX 86400

########################################################################
#
# utility routines

proc die {{msg ""}} { if {$msg ne ""} { warn $msg }; exit 1 }

proc shift {list} {
    upvar 1 $list ll
    set ll [lassign $ll res]
    return $res
}

proc warn {msg} { puts stderr $msg }

########################################################################
#
# business logic

proc audit_hostnames {args} {
    global accept_fqdn
    if {$accept_fqdn} {
        reject_invalid_fqdn {*}$args
    } else {
        reject_invalid_subdomain {*}$args
    }
}

proc positive_int_or {name default} {
    upvar 1 $name value
    if {$value eq ""} {
        set value $default
    } else {
        if {![string is integer $value] || $value < 0} {
            die "$name must be a positive integer"
        }
    }
}

# [RFC 1035] section 2.3.4 and [RFC 1123] section 2.1 for hosts
# (which cannot have _ unlike SRV records). i18n data must already be
# in punycode form [RFC 5891]. terminology where possible taken from
# [RFC 7719]

# FIXME some sites may want to limit $domain to be only particular names
# (to prevent typos from going to unknown places, or...)
proc reject_invalid_domain {} {
    global domain
    if {[string index $domain end] ne "."} {
        die "\$domain must end with a ."
    }
    # NOTE in contrast to FQDN must preserve trailing . on $domain
    set dom [string range $domain 0 end-1]
    foreach label [split $dom "."] {
        if {[string length $label] == 0} {
            die "$name labels must have some length"
        }
        reject_invalid_host_label $domain $label
    }
}

# fully qualified name such as "foo.bar.example.net." (this ignores $domain)
# expects to be called via audit_hostnames
proc reject_invalid_fqdn {args} {
    foreach name $args {
        upvar 2 $name value
        if {[string length $value] > 253} {
            die "$name cannot be longer than 253 characters"
        }
        if {[string index $value end] ne "."} {
            die "$name must end with a ."
        }
        # pretty sure folks won't be editing "." zone
        if {[string length $value] < 2} {
            die "$name must be longer than a character"
        }
        # NOTE the trailing . is removed here for both the label split
        # and for when building $nsupdate
        set value [string range $value 0 end-1]
        foreach label [split $value "."] {
            if {[string length $label] == 0} {
                die "$name labels must have some length"
            }
            reject_invalid_host_label $name $label
        }
    }
}

# subdomain such as "zot" or "foo.bar" under the $domain
# expects to be called via audit_hostnames
proc reject_invalid_subdomain {args} {
    reject_invalid_domain
    global domain
    foreach name $args {
        upvar 2 $name value
        if {[string length $value] + 1 + [string length $domain] > 253} {
            die "$name cannot be longer than 253 characters"
        }
        if {[string index $value end] eq "."} {
            die "$name must not end with a ."
        }
        if {$value eq ""} {
            die "$name cannot be empty string"
        }
        foreach label [split $value "."] {
            if {[string length $label] == 0} {
                die "$name labels must have some length"
            }
            reject_invalid_host_label $name $label
        }
    }
}

# a hostname label such as "bar" of "foo.bar.example.net"
proc reject_invalid_host_label {name value} {
    if {[string length $value] > 63} {
        die "$name label cannot be longer than 63 characters"
    }
    if {![regexp -nocase {^([a-z0-9]+|[a-z0-9][a-z0-9-]+[a-z0-9])$} $value]} {
        die "$name label may only contain /a-z0-9/i or with hyphen in middle"
    }
}

# or instead with (at least leading) underscores
proc allow_underscore_hosts {} {
    proc reject_invalid_host_label {name value} {
        if {[string length $value] > 63} {
            die "$name label cannot be longer than 63 characters"
        }
        if {![regexp -nocase {^([_a-z0-9]+|[a-z0-9][a-z0-9-]+[a-z0-9])$} $value]} {
            die "$name label may only contain /a-z0-9/i or with hyphen in middle"
        }
    }
}
