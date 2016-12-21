#!perl

use 5.14.0;
use warnings;
use Expect;
use File::Temp qw(tempfile);
use Test::Most tests => 4;

my $test_prog = 'is-changed-mtime';

my ( $tfh, $test_file );
lives_ok(
    sub {
        ( $tfh, $test_file ) = tempfile( "icm.XXXXXXXXXX", TMPDIR => 1, UNLINK => 1 );
    },
    'tempfile creation should not croak'
);

my $exp = newexpect();
ok( $exp->spawn( $test_prog, '5s', "$^X -E 'say q{poke}'", $test_file ),
    "expect object spawned: $!" );

diag "tests will take some time...";

# NOTE KLUGE timing might be off on a busy system, or possibly buffering
# related things, PORTABILITY of the utime calls, etc.
sleep 1;
utime undef, undef, $test_file;
sleep 1;
utime undef, undef, $test_file;
sleep 5;

kill INT => $exp->pid;

my $return;
$exp->expect(
    10,
    'eof'     => sub { $return = 'eof' },
    'timeout' => sub { $return = 'timeout' }
);
is( $return, 'eof', "exits after control+c" );

my $pokes;
$pokes++ for $exp->before =~ m/poke/g;
is( $pokes, 1, "one poke from two touches" );

sub newexpect {
    my $exp = Expect->new;
    $exp->raw_pty(1);
    #$exp->debug(3);
    return $exp;
}
