#!perl
#
# NOTE these tests make assumptions about the mount points that may be
# unportable or may fail if the mount points change during the tests

use lib qw(../lib/perl5);
use UtilityTestBelt;

my $cmd = Test::UnixCmdWrap->new;

my $newl = $cmd->run( stdout => qr{/} )->stdout                  =~ s/\n/ /gr;
my $null = $cmd->run( args   => '-0N', stdout => qr{/} )->stdout =~ s/\0/ /gr;
diag "mount points: $newl";
is( $newl, $null );

# -q does not affect listing of filesystems mode
$cmd->run( args => '-q', stdout => qr{/} );

$cmd->run(
    args   => '/',
    stdout => ['/'],
);
$cmd->run( args => '-q /' );

$cmd->run(
    args   => '-m',
    status => 64,
    stderr => qr/Usage/
);
$cmd->run(
    args   => 'too many',
    status => 64,
    stderr => qr/Usage/
);
$cmd->run(
    args   => '""',
    status => 64,
    stderr => qr/Usage/
);
$cmd->run( args => '-h', status => 64, stderr => qr/Usage/ );

done_testing(28);
