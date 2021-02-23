#!perl
use lib qw(../lib/perl5);
use UtilityTestBelt;

my $cmd = Test::UnixCmdWrap->new;

my $tdir = tempdir( "thisdir.XXXXXXXXX", CLEANUP => 1, TMPDIR => 1 );

my $file = "$tdir/diag";
open my $fh, '>', $file or die "could not write '$file': $!\n";
print $fh <<"EOF";
#!$^X
use feature 'say';
say \$0;
say for \@ARGV;
say 'THISDIR=' . \$ENV{THISDIR};
EOF
close $fh;
chmod 0700, $file or die "chmod failed??\n";

# always is set so does not test the default, test only tests that
# something did not remove the value
#
# TODO build a test version of the program with a custom HOME set?
$ENV{THISDIR} = $tdir;

$cmd->run(
    args   => 'diag',
    stdout => qr{(?s)/diag.+scripts/filesys.+THISDIR=$tdir}
);

$cmd->run(
    args   => "diag a$$ b$$ c$$",
    stdout => qr{(?s)/diag.+scripts/filesys.+a$$.+b$$.+c$$}
);

$cmd->run(
    args   => "../naughty",
    status => 1,
    stderr => qr/must not contain/
);
$cmd->run(
    args   => "foo/../naughty",
    status => 1,
    stderr => qr/must not contain/
);

$cmd->run(
    args   => "no/..such..mode-$$",
    status => 1,
    stderr => qr/exec/
);

$cmd->run(
    args   => '""',
    status => 64,
    stderr => qr/Usage/
);
$cmd->run( args => '-h', status => 64, stderr => qr/Usage/ );

done_testing(21);
