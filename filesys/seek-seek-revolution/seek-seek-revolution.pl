#!/usr/bin/env perl

use Time::HiRes qw(sleep);

open my $fh, '+>', 'revolution' or die "make revolution writable";
print $fh "echo say what\nsleep 1\n";
seek $fh, 0, 0;

my $pid = fork() // die "nope: $!\n";

if ($pid == 0) {
    close STDIN;
    open STDIN, '<&', $fh;
    exec 'sh';
} else {
    while (1) { seek $fh, 0, 0; sleep 0.1 }
}
