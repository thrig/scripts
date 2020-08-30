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

$cmd->run(
    args   => "-4 $lo0",
    stdout => ["127.0.0.1"]
);

$o = $cmd->run(
    args   => "-4 -R $lo0",
    stdout => qr/^/,
);
my ($host) = $o->stdout =~ m/^(.*)/;
diag "127.0.0.1 -> '$host'";

$cmd->run(
    args   => "hopefully-does-not-exist-$$",
    status => 2
);

$cmd->run(args => '-h', status => 64, stderr => qr/Usage/);

done_testing(15);
