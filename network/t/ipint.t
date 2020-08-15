#!perl
use lib qw(../lib/perl5);
use UtilityTestBelt;

my $cmd = Test::UnixCmdWrap->new;

$cmd->run(
    args   => '127.0.0.1',
    stdout => ["2130706433"],
);
$cmd->run(
    args   => '2130706433',
    stdout => ["127.0.0.1"],
);
# +prefix is allowed to strtoul
$cmd->run(
    args   => '+42',
    stdout => ["0.0.0.42"],
);
# but not negatives as those get too big for uint32_t
$cmd->run(args => '-- -42',        status => 65, stderr => qr/positive/);
$cmd->run(args => '" -42"',        status => 65, stderr => qr/positive/);
$cmd->run(args => 'livore',        status => 65, stderr => qr/positive/);
$cmd->run(args => 'multiple args', status => 64, stderr => qr/Usage/);
$cmd->run(args => '',              status => 64, stderr => qr/Usage/);
$cmd->run(args => '-h',            status => 64, stderr => qr/Usage/);
$cmd->run(status => 64, stderr => qr/Usage/);

done_testing(30);
