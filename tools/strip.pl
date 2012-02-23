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

sub remove_keywords {

  my ($lines, $func, $keywords) = @_;

  my $found;
  my $pass = 0;
  my $line;
  my $mod_line = "";
  my @mod_lines = ();

  foreach $line (@{$lines}) {

    if ($pass) {
      push (@mod_lines, $line);
    } else {
      my $i = 0;
      my $n = 0;
      my $s = "";
      my @chrs = split (//, $line);
      my $nchrs = scalar @chrs;
      $mod_line = "";

      for ($i = 0; $i < $nchrs; ) {
        if ($pass) {
          $mod_line .= $chrs[$i++];
        } else {
          $found = 0;
          $n = length $func;
          if (($nchrs - ($i+1)) >= $n) {
            $s = join ("", @chrs[$i..($i+($n-1))]);
            if ($s eq $func) {
              $i += $n;
              $mod_line .= $s;
              $pass = 1;
              next;
            }
          }

          foreach my $keyword (@{$keywords}) {
            $n = length $keyword;
            if (($nchrs - ($i+1)) >= $n) {
              $s = join ("", @chrs[$i..($i+($n-1))]);
              if ($s eq $keyword) {
                $found = 1;
                $i += $n;
                last;
              }
            }
          }

          if (! $found) {
            $mod_line .= $chrs[$i++];
          }
        }
      }

      push (@mod_lines, $mod_line);
    }
  }

  return wantarray ? @mod_lines : \@mod_lines;
}

sub remove_functions {

  my ($lines, $funcs) = @_;

  my $found = "";
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
            my @keywords = ();
            foreach my $keyword (keys %{$funcs->{$found}}) {
              if ($keyword =~ m/function/) {
                @keywords = ();
                last;
              } else {
                push (@keywords, $keyword);
              }
            }

            if (scalar (@keywords)) {
              push (@line_buffer, $line);
              push (@mod_lines, remove_keywords (\@line_buffer, $found, \@keywords));
            }
            $found = "";            
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
        $found = "";
        foreach $func (keys %{$funcs}) {
          my $n = length $func;
          next if (($nchrs - ($i+1)) < $n);
          my $s = join ("", @chrs[$i..($i+($n-1))]);

          if ($s eq $func) {
            if ($nbrackets == 0 && $ncurly_brackets == 0) {
              $found = $func;
              $in_func = 1;
              $i += $n;
              last;
            } elsif ($funcs->{$func}->{function}) {
              error ("reference to %s on line %u", $func, $nline);
            }
          }
        }

        $i++ if (! $found);
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
 --remove-keyword-static=FUNC
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
  my @rm_keywords = ();

  GetOptions ('force' => \$force,
              'remove-line-control' => \$rm_line_ctl,
              'remove-function=s@' => \@rm_funcs,
              'remove-keyword-static=s@' => \@rm_keywords);

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

  my %kws = ();

  foreach my $kw (@rm_funcs) {
    if ($kws{$kw}) {
      $kws{$kw}->{function} = 1;
    } else {
      $kws{$kw} = {};
      $kws{$kw}->{function} = 1;
    }
  }

  foreach my $kw (@rm_keywords) {
    if ($kws{$kw}) {
      $kws{$kw}->{static} = 1;
    } else {
      $kws{$kw} = {};
      $kws{$kw}->{static} = 1;
    }
  }

  if (scalar (keys (%kws))) {
    @lines = remove_functions (\@lines, \%kws);
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

