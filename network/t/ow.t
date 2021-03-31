#!perl
#
# NOTE if the tests go awry w3m(1) or otherwise the DEFAULT_COMMAND may
# be launched...

use lib qw(../lib/perl5);
use DB_File;
use UtilityTestBelt;

my $cmd = Test::UnixCmdWrap->new;

-f "wv" or link qw(ow wv) or BAIL_OUT("link failed: $!");
my $wv = Test::UnixCmdWrap->new( cmd => './wv' );

my $test_dir = tempdir( 'ow.XXXXXXXXXX', CLEANUP => 1, TMPDIR => 1 );

# this should use the $ow_dir configuration. I hope.
$ENV{HOME} = $test_dir;

########################################################################
#
# SETUP of various necessary config files and databases

my $ow_dir = catdir( $test_dir, '.ow' );
mkdir $ow_dir or BAIL_OUT("mkdir .ow failed: $!");

my $fh;
open $fh, '>', catfile( $ow_dir, 'dirmap' )
  or BAIL_OUT("write dirmap failed: $!");
say $fh "(?<x>scripts)/network X%{x}%{nope}%{x}Y";
close $fh;

open $fh, '>', catfile( $ow_dir, 'remap' )
  or BAIL_OUT("write remap failed: $!");
say $fh "mirsna echo2";
close $fh;

# makemap(8) or whatever is not portable, so directly write these, with
# appropriate addition of \0 (see DB_File perldoc for details)
my %browsers;
tie %browsers, 'DB_File', catfile( $ow_dir, 'browsers.db' );
# NOTE this assumes that echo(1) is available in the PATH somewhere. the
# length is to ensure that some realloc code is at least touched on
$browsers{"echo2\0"} = "echo e e e e e e e e e e e e e e e e e e e e\0";
untie %browsers;

my %shortcuts;
tie %shortcuts, 'DB_File', catfile( $ow_dir, 'shortcuts.db' );
$shortcuts{"foo\0"}    = "bar\0";
$shortcuts{"foo@\0"}   = "X%\@Y\0";
$shortcuts{"foo21@\0"} = "P%2-%1Q\0";
$shortcuts{"*\0"}      = "wildcard\0";
# TODO what would this do? probably do a web search with the given
# args... need to implement in the *.c if do support this
#$shortcuts{"*@\0"} = "wildcard\0";
untie %shortcuts;

########################################################################
#
# TESTS

# some shortcuts
$cmd->run(
    args   => "-l foo",
    stdout => qr{^bar$},
);
$cmd->run(
    args   => "-l foo a b c",
    stdout => qr{^Xa\+b\+cY$},
);
# TODO a better template system might error or warn if there are too few
# or too many template arguments given. presently any missing slots are
# replaced with the empty string, for better or worse
#
# %@
$cmd->run(
    args   => "-l foo21 a b c",
    stdout => qr{^Pb-aQ$},
);
# %1 %2 and such
$cmd->run(
    args   => "-l foo21 a",
    stdout => qr{^P-aQ$},
);
$cmd->run(
    args   => "-l foo21 a",
    stdout => qr{^P-aQ$},
);
# fallthrough to * entry
$cmd->run(
    args   => "-l unknown",
    stdout => qr{^wildcard$},
);

# custom -o and that what looks like a URL is passed through
$cmd->run(
    args   => "-o echo gopher://example",
    stdout => qr{^gopher://example$},
);

# that a "hostname" gets URLified and that a command remap happens
$cmd->run(
    args   => "mirsna.example.org",
    stdout =>
      qr{^e e e e e e e e e e e e e e e e e e e e https://mirsna.example.org$},
);

# -d generates file:// from PWD lacking a dirmap entry
$cmd->run(
    args   => "-dl",
    chdir  => $test_dir,
    stdout => qr{^file://$test_dir$}
);
# filename argument (realpath(3) may fail if this does not exist)
$cmd->run(
    args   => "-dl .ow",
    chdir  => $test_dir,
    stdout => qr{^file://${test_dir}/.ow$}
);
# custom dirmap
$cmd->run( args => "-dl", stdout => qr/^XscriptsscriptsY/ );

# help or various invalid usages
$cmd->run(
    status => 64,
    stderr => qr/^Usage/,
);
$cmd->run(
    args   => "-h",
    status => 64,
    stderr => qr/^Usage/,
);
$cmd->run(
    args   => "-X",
    status => 64,
    stderr => qr/Usage/,
);
$cmd->run(
    args   => "-d too many",
    status => 64,
    stderr => qr/^Usage/,
);
$wv->run(
    args   => "too many",
    status => 64,
    stderr => qr/^Usage/,
);

done_testing 48
