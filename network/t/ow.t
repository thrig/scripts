#!perl
#
# NOTE some tests assume that echo(1) is available in the PATH somewhere
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
say $fh <<'DIRMAP';
(?<x>scripts)/network X%{x}%{nope}%{x}Y

# tag tests (and a comment line test, too)
# the line before the previous line was intentionally left blank
(?<x>scripts)/network D%{tag}D dev
(?<x>scripts)/network P%{tag}P prod
DIRMAP
close $fh;

open $fh, '>', catfile( $ow_dir, 'remap' )
  or BAIL_OUT("write remap failed: $!");
say $fh <<'REMAP';

# echo echo
mirsna echo2

# echo

REMAP
close $fh;

# makemap(8) or whatever is not portable, so directly write these, with
# appropriate addition of \0 (see DB_File perldoc for details)
my ( %browsers, %shortcuts );

tie %browsers, 'DB_File', catfile( $ow_dir, 'browsers.db' );
# the length is to ensure that some realloc code is at least touched on
$browsers{"echo2\0"} = "echo e e e e e e e e e e e e e e e e e e e e\0";
untie %browsers;

my $sdb = tie %shortcuts, 'DB_File', catfile( $ow_dir, 'shortcuts.db' );
$shortcuts{"foo\0"}    = "bar\0";
$shortcuts{"foo@\0"}   = "X%\@Y\0";
$shortcuts{"foo21@\0"} = "P%{2}-%{1}Q\0";
$shortcuts{"tag\0"}    = "Ts%{tag}sT\0";
$shortcuts{"tag@\0"}   = "T%{tag}T\0";
# these are optional and may not be desirable (this will depend on how
# you feel about random typos of your other shortcuts going to probably
# some search engine)
$shortcuts{"*\0"}  = "wildcard\0";
$shortcuts{"*@\0"} = "wildcard-%@\0";

$sdb->sync;

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
$cmd->run(
    args   => "-l foo21 a b c",
    stdout => qr{^Pb-aQ$},
);
# %{1} %{2} and such
$cmd->run(
    args   => "-l foo21 a",
    stdout => qr{^P-aQ$},
);
$cmd->run(
    args   => "-l foo21 a",
    stdout => qr{^P-aQ$},
);
# tags
$cmd->run(
    args   => "-t dev -l tag",
    stdout => qr{^TsdevsT$},
);
$cmd->run(
    args   => "-t dev -l tag a",
    stdout => qr{^TdevT$},
);
$cmd->run(
    args   => "-t prod -l tag a",
    stdout => qr{^TprodT$},
);
# fallthrough to * entry
$cmd->run(
    args   => "-l unknown",
    stdout => qr{^wildcard$},
);
# fallthrough to *@ entry
$cmd->run(
    args   => "-l unknown a b",
    stdout => qr{^wildcard-unknown\+a\+b$},
);

$cmd->run(
    args   => "unknown",
    env    => { OW_COMMAND => "echo foobar" },
    stdout => qr{^foobar wildcard$},
);

# drop the wildcard entries and test the default failure case
delete $shortcuts{$_} for "*\0", "*@\0";
$sdb->sync;

$cmd->run(
    args   => "-l unknown",
    status => 1,
    stderr => qr/not sure what to do with/,
);
# fallthrough to *@ entry
$cmd->run(
    args   => "-l unknown a b",
    status => 1,
    stderr => qr/not sure what to do with/,
);

# custom -o and that what looks like a URL is passed through
$cmd->run(
    args   => "-o echo gopher://example",
    stdout => qr{^gopher://example$},
);

# that a "hostname" gets URLified and that a command remap happens
$cmd->run(
    args => "mirsna.example.org",
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
$cmd->run( args => "-dl", stdout => qr/^XscriptsscriptsY$/ );

# tags in dirmap
$cmd->run( args => "-t dev -dl",  stdout => qr/^DdevD$/ );
$cmd->run( args => "-t prod -dl", stdout => qr/^PprodP$/ );

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

done_testing 75
