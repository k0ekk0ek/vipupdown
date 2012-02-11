#!/usr/bin/perl
# strip.pl - take ifupdown source file and prepare it for inclusion in vipupdown

use strict;
use utf8;
use warnings;
use Getopt::Long;

sub error {

  my $fmt = shift (@_);

  if ($fmt) {
    $fmt =~ s/\s*$//o;
    $fmt =~ s/^\s*//o;
  }

  if ($fmt) {
    printf STDERR "$fmt\n", @_;
  } else {
    printf STDERR "unknown error\n";
  }

  exit (1);
}

sub remove_line_control {

  my ($lines) = @_;

  my $line;
  my @mod_lines = ();

  foreach $line (@{$lines}) {
    if ($line =~ m/^#line/) {
      push (@mod_lines, "\n");
    } else {
      push (@mod_lines, $line);
    }
  }

  return wantarray () ? @mod_lines : \@mod_lines;
}

use constant STATE_NONE => 0;
use constant STATE_FUNC_NAME => 1;
use constant STATE_IN_FUNC => 2;

sub remove_functions {

  my ($lines, $funcs) = @_;

  my $in_preproc = 0;
  my $in_func = 0;
  my $nbrackets = 0;
  my $ncurly_brackets = 0;
  my $end_of_statement = 0;
  my $line;
  my $nline = 0;
  my @line_buffer = ();
  my @mod_lines = ();

  foreach $line (@{$lines}) {

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
            push (@mod_lines, @line_buffer);
            push (@mod_lines, $line);
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
        foreach $func (@{$funcs}) {
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
              error ("reference to %s on line %u", $func, $nline);
            }
          }
        }

        $i++ if ($found == 0);
      } else {
        $i++;
      }
    }
  }

  return wantarray () ? @mod_lines : \@mod_lines;
}

sub usage {

  my $fmt = <<EOF;
Usage: %s [OPTION] SRC DEST

Options
 --remove-line-control
 --remove-function=FUNC
EOF

  printf $fmt, $0;
  exit 0;
}

sub main {

  my $force;
  my $src_file;
  my $dest_file;
  my $rm_line_ctl = 0;
  my @rm_funcs = ();

  GetOptions ('force' => \$force,
              'remove-line-control' => \$rm_line_ctl,
              'remove-function=s@' => \@rm_funcs);

  $src_file = $ARGV[0] || '';
  $dest_file = $ARGV[1] || '';

  usage () if (! $src_file || ! -e $src_file);
  usage () if (! $dest_file);

  error ("%s already exists, specify --force to overwrite\n", $dest_file)
    if (-e $dest_file && ! $force);

  # read file into memory
  my $line;
  my @lines = ();
  my $src_handle;
  my $dest_handle;

  error ("open: %s: %s", $src_file, $!)
    if (! open ($src_handle, '<', $src_file));

  while ($line = <$src_handle>) {
    push (@lines, $line);
  }

  close ($src_handle);

  # modify source
  if ($rm_line_ctl) {
    @lines = remove_line_control (\@lines);
  }

  if (scalar (@rm_funcs)) {
    @lines = remove_functions (\@lines, \@rm_funcs);
  }

  # write file to disk
  error ("open: %s: %s", $dest_file, $!)
    if (! open ($dest_handle, '>', $dest_file));

  foreach $line (@lines) {
    print $dest_handle $line;
  }

  close ($dest_handle);

  return 0;
}

exit main;

