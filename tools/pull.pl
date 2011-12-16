#!/usr/bin/perl
# pull.pl - take ifupdown sources and prepare them for inclusion in vifupdown.

use strict;
use utf8;
use warnings;

#int execute_all(interface_defn *ifd, execfn *exec, char *opt);
#
#int iface_up(interface_defn *iface);
#int iface_down(interface_defn *iface);
#
#int execute(char *command, interface_defn *ifd, execfn *exec);


my $line_control_pattern = '^#line\s+\d+\s+"ifupdown.nw"';
my %files = (
  'config.c'  => { 'remove_line_control' => 1,
                   'remove_functions'    => [ ] },
  'execute.c' => { 'remove_line_control' => 1,
                   'remove_functions'    => [ ] },
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

sub remove_functions {

  my ($in_fn, @funcs) = @_;

  # this function will destroy all functions and function prototypes as given
  # in funcs!

  my $state = 0;

  my $in_fh = undef;
  if (! open ($in_fh, '<', $in_fn)) {
    # error
  }

  while ($line = <$infh>) {
    my @chrs = split (//, $line);
    my $nchrs = scalar @chrs;

    for ($i = 0; $i < $nchrs; $i++) {
      # implement...
      # using just a '{|}' counter seems like an easy way to do what I want!
      #if ($state == $state_in_func) {
      #  #if ($chrs[$i]
      #
      #} elsif ($state == $state_in_
    }
  }
}

sub main {

  foreach my $file (keys (%files)) {
    remove_line_control ("$file", "$file.sane");
  }

  return 0;
}

exit main;
