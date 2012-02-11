#!/usr/bin/perl
# clean.pl - take ifupdown source file and prepare it for inclusion in vipupdown

use strict;
use utf8;
use warnings;

my $line_control_pattern = '^#line\s+\d+\s+"ifupdown.nw"';
my %files = (
  'config.c'  => { 'remove_line_control' => 1,
                   'remove_functions'    => [ ] },
  'execute.c' => { 'remove_line_control' => 1,
                   'remove_functions'    => ['execute_all', 'iface_up', 'iface_down', 'execute'] },
  'header.h'  => { 'remove_line_control' => 1,
                   'remove_functions'    => ['execute_all', 'iface_up', 'iface_down', 'execute'] }
);

sub subroutine {

  return (caller (1))[3];
}

sub remove_line_control {

  my ($in, $out) = @_;

  my $infh = undef;
  my $outfh = undef;

  if (! open ($infh, '<', $in)) {
    printf STDERR "%s: open %s: %s\n", subroutine, $in, $!;
    goto error;
  }
  if (! open ($outfh, '>', $out)) {
    printf STDERR "%s: open %s: %s\n", subroutine, $out, $!;
    goto error;
  }

  my $line;
  while ($line = <$infh>) {
    if ($line =~ m/$line_control_pattern/) {
      print $outfh "\n";
    } else {
      print $outfh $line;
    }
  }

  close ($infh);
  close ($outfh);

  return 0;

error:
  close ($infh)
    if (defined $infh);
  close ($outfh)
    if (defined $outfh);
  return -1;
}

use constant STATE_NONE => 0;
use constant STATE_FUNC_NAME => 1;
use constant STATE_IN_FUNC => 2;

sub remove_functions {

  my ($in, $out, @funcs) = @_;

  my $infh = undef;
  my $outfh = undef;

  if (! open ($infh, '<', $in)) {
    printf STDERR "%s: open %s: %s", subroutine, $in, $!;
    goto error;
  }
  if (! open ($outfh, '>', $out)) {
    printf STDERR "%s: open %s: %s", subroutine, $out, $!;
    goto error;
  }

  my $in_preproc = 0;
  my $in_func = 0;
  my $nbrackets = 0;
  my $ncurly_brackets = 0;
  my $end_of_statement = 0;
  my $line;
  my $nline = 0;
  my @line_buffer = ();

  while ($line = <$infh>) {

    $nline++;

    my $i;
    my @chrs = split (//, $line);
    my $nchrs = scalar @chrs;

    for ($i = 0; $i < $nchrs; ) {
      if ($chrs[$i] eq "#") {
        $in_preproc = 1;
        $i++;
      } elsif ($chrs[$i] eq "\\") {
        $in_preproc++ if ($in_preproc);
        $i++;
      } elsif ($chrs[$i] eq "(") {
        $nbrackets++;
        $i++;
      } elsif ($chrs[$i] eq ")") {
        $nbrackets--;
        $i++;
      } elsif ($chrs[$i] eq "{") {
        $ncurly_brackets++;
        $i++;
      } elsif ($chrs[$i] eq "}") {
        $ncurly_brackets--;
        $end_of_statement = 1 if ($ncurly_brackets == 0);
        $i++;
      } elsif ($chrs[$i] eq ";") {
        $end_of_statement = 1;
        $i++;
      } elsif ($chrs[$i] eq " " || $chrs[$i] eq "\t") {
        # ignore white space
        $i++;
        next;
      } elsif ($chrs[$i] eq "\n") {
        if ($in_preproc) {
          $in_preproc--;
          $end_of_statement = 1 if ($in_preproc == 0);
        }

        if ($nbrackets == 0 && $ncurly_brackets == 0 && $in_preproc == 0 && $end_of_statement) {
          if ($in_func) {
            $in_func = 0;
            @line_buffer = ();
          } else {
            print $outfh @line_buffer;
            print $outfh $line;
            @line_buffer = ();
          }
        } else {
          push (@line_buffer, $line);
        }
        $i++;
      } elsif ($in_func == 0) {
        $end_of_statement = 0;

        # possible prototype or implementation
        my $func;
        my $found = 0;
        foreach $func (@funcs) {
          my $n = length $func;
          next if (($nchrs - ($i+1)) < $n);
          my $s = join ("", @chrs[$i..($i+($n-1))]);

          if ($s eq $func) {
            if ($nbrackets == 0 && $ncurly_brackets == 0) {
              $found = 1;
              $in_func = 1;
              $i += $n;
              last;
            } else {
              printf STDERR "reference to $func in $in line $nline\n";
              goto error;
            }
          }
        }

        $i++ if ($found == 0);
      } else {
        $i++;
      }
    }
  }

  close ($infh);
  close ($outfh);
  return 0;

error:
  close ($infh)
    if (defined $infh);
  close ($outfh)
    if (defined $outfh);
  return -1;
}

sub main {

  return 1
    if (@ARGV < 2 || $ARGV[0] eq $ARGV[1]);

  remove_line_control ($ARGV[0], $ARGV[1].".0");
  remove_functions ($ARGV[1].".0", $ARGV[1].".1", 'execute_all', 'iface_up', 'iface_down', 'execute');
  rename ($ARGV[1].".1", $ARGV[1]);
  unlink ($ARGV[1].".0");

  return 0;
}

exit main;

