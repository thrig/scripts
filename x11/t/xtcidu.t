#!perl
use lib qw(../lib/perl5);
use UtilityTestBelt;

# KLUGE but do kind of need X11 for most of the tests
if (!exists $ENV{DISPLAY}) {
    diag "DISPLAY is not set... is X11 setup?\n";
    print "Bail out!\n";
}

my $cmd = Test::UnixCmdWrap->new;

diag q{type "cat"};
my $o = $cmd->run( stdout => qr/^cat$/ );
ok( $o->stdout =~ m/\n/ );

diag q{type "cat" (and confirm that "/" is not accepted)};
$cmd->run( args => "-B /", stdout => qr/^cat$/ );

diag q{type "cat" again};
$o = $cmd->run( args => "-n", stdout => qr/^cat$/ );
ok( $o->stdout !~ m/\n/ );

$cmd->run( args => '-h', status => 64, stderr => qr/Usage/ );

done_testing(14);
