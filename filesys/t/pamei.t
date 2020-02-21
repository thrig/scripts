#!perl
use lib qw(../lib/perl5);
use POSIX qw(setsid);
use UtilityTestBelt;

my $cmd      = Test::UnixCmdWrap->new;
my $prog     = $cmd->cmd->prog;
my $test_dir = tempdir('pamei.XXXXXXXXXX', CLEANUP => 1, TMPDIR => 1);
$ENV{PAMEI_DIR} = $test_dir;

sub sleeper {
    my $pid = fork();
    BAIL_OUT("fork failed: $!") unless defined $pid;
    if ($pid == 0) {
        setsid();
        # though this will probably muss up the TAP output lots when
        # it fails
        exec($prog, 'slee/per', $^X, '-e', 'sleep 59')
          or die "exec failed: $!\n";
    }
    return $pid;
}

diag "tests may take some time...";

my $child = sleeper();
sleep 3;
ok -f catfile($test_dir, 'slee_per');

$cmd->run(
    args   => "slee/per '$^X' -e 'print qq{whoops}'",
    status => 2,
    stderr => qr/resource is locked/
);
kill '-TERM' => $child;
sleep 3;
$cmd->run(
    args   => "slee/per '$^X' -e 'print qq{ok\n}'",
    stdout => "ok\n"
);

$cmd->run(status => 64, stderr => qr/^Usage: /);
$cmd->run(args => "notenough", status => 64, stderr => qr/^Usage: /);

# this will likely fail if the test is run as root
my $echop = catfile($test_dir, 'echo');
open my $fh, '>', $echop or BAIL_OUT("could not touch $echop: $!");
close $fh;
chmod 0, $echop or BAIL_OUT("chmod failed: $!");
$cmd->run(
    args   => "echo echo echo",
    status => 74,
    stderr => qr/open failed /
);

# hopefully does not exist, despite certain vendors putting files into
# /var/empty ...
$ENV{PAMEI_DIR} = "/var/empty/$$/nope/" . int rand 2147483646;
$cmd->run(
    args   => "echo echo echo",
    status => 74,
    stderr => qr/could not stat /
);

done_testing 19
