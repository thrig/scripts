#!perl
use lib qw(../../lib/perl5);
use UtilityTestBelt;

my $cmd = Test::UnixCmdWrap->new;

$cmd->run(args => '-h', stderr => qr/Usage/, status => 64,);

# no input is no foul; perhaps it should instead signal an error?
# (perhaps via an -A abort flag?)
$cmd->run;

$cmd->run(args => 't/tally-input', stdout => [ "2 foo", "1 bar" ]);

$cmd->run(
    args   => 't/tally-input t/tally-input',
    stdout => [ "4 foo", "2 bar" ]
);

$cmd->run(stdin => <<'EOF', stdout => [ "2 foo", "1 bar" ]);
bar
foo
foo
EOF

$cmd->run(
    args   => '-t',
    stderr => qr/^tally: unique 2 of total 3/,
    stdin  => <<'EOF', stdout => [ "2 foo", "1 bar" ]);
bar
foo
foo
EOF

my $input = '';
my %tally;
for (1 .. 100) {
    my $n = int(3 + rand 6 + rand 6 + rand 6);
    $input .= $n . $/;
    $tally{$n}++;
}
my $o = $cmd->run(stdin => $input, stdout => qr/^/,);
# NOTE must use the same sort on both outputs as qsort(3) may sort lines
# that appear the same number of times differently than a Perl or shell
# implementation (the C only sorts by the counts and does not subsort by
# the content of the line)
eq_or_diff([ sort split $/, $o->stdout ],
    [ sort map { "$tally{$_} $_" } keys %tally ]);

done_testing 22
