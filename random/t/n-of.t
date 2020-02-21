#!perl
use lib qw(../lib/perl5);
use UtilityTestBelt;

my $cmd  = Test::UnixCmdWrap->new;
my $tcmd = $cmd->cmd;

# these are basic interface tests; more than one input line requires
# statistical guesswork on account of the (hopefully) random nature of
# the line selection
$cmd->run(
    args   => "3",
    stdin  => "one\n",
    stdout => "one\n",
);
$cmd->run(
    args   => "3",
    stdin  => "one\ntwo\nthree",
    stdout => "one\ntwo\nthree\n",
);
$cmd->run(
    args   => "1",
    stdin  => "nonl",
    stdout => "nonl\n",
);
# invalid stuff
$cmd->run(status => 64,  stderr => qr/^Usage/);
$cmd->run(args   => "1", status => 1);
# actually from goptfoo library
$cmd->run(
    args   => "'0'",
    stdin  => "what\never\n",
    status => 65,
    stderr => qr/count value is below min/
);

# may false alarm if the RNG rolls... poorly
my %seen;
for (1 .. 50) {
    $tcmd->run(args => "2", stdin => "a\nb\nc\nd\ne\n");
    for my $line (split /\n/, $tcmd->stdout) {
        chomp $line;
        $seen{$line}++;
    }
}
eq_or_diff([ sort keys %seen ], [qw/a b c d e/]);

# shuffle
%seen = ();
for (1 .. 60) {
    $tcmd->run(args => "-s 2", stdin => "a\nb\nc");
    $seen{ $tcmd->stdout }++;
}
eq_or_diff([ sort keys %seen ],
    [ "a\nb\n", "a\nc\n", "b\na\n", "b\nc\n", "c\na\n", "c\nb\n" ]);

done_testing 20
