#!perl
use lib qw(../lib/perl5);
use UtilityTestBelt;

diag("NOTE these tests may be unportable");

my $cmd = Test::UnixCmdWrap->new;

my $o = $cmd->run(
    args   => '',
    stdout => qr/127.0.0.1/
);
my ($lo0) = $o->stdout =~ m/(\S+) 127.0.0.1/;

SKIP: {
    skip "no IPv6 on $lo0 ?", 3 unless $o->stdout =~ m/$lo0 ::1/;
    $cmd->run(
        args   => "-6 $lo0",
        stdout => qr/(?m)^::1$/,
    );
}

# this test will fail if there are other addresses on loopback (like
# 127.0.0.2 for testing, or ...)
$cmd->run(
    args   => "-4 $lo0",
    stdout => ["127.0.0.1"]
);

# who knows what the DNS setup is like
$o = $cmd->run(
    args   => "-4 -R $lo0",
    stdout => qr/^/,
);
my ($host) = $o->stdout =~ m/^(.*)/;
diag "'$lo0' -> 127.0.0.1 -> '$host'";

$cmd->run(
    args   => "hopefully-does-not-exist-$$",
    status => 2
);

$cmd->run(args => 'too many args', status => 64, stderr => qr/Usage/);
$cmd->run(args => '-h',            status => 64, stderr => qr/Usage/);

done_testing 21
