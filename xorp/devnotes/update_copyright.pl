#!/usr/bin/perl

use strict;

if (@ARGV < 2) {
  print "Usage: update_copyright.pl <year> <file>\n";
  print "Example: update_copyright.pl 2011 foo.cc\n";
  exit(1);
}

my $year = $ARGV[0];
my $fname = $ARGV[1];
my $tmpfname = "/tmp/xorp_copyright_update.tmp";
my $found_one = 0;
my $next_xcr = 0;

# Skip some files we should never directly update.
if ($fname =~ /^docs\/wiki\//) {
  print "Skipping wiki file: $fname\n";
  exit(0);
}

open(FH, "<", $fname) or die "Can't open file: $fname  : $!";
open(OUTF, ">", "$tmpfname") or die "Can't open output file: $tmpfname  : $!";

while(<FH>) {
  my $ln = $_;
  if ($ln =~ /Copyright \(c\) (\d\d\d\d)-(\d\d\d\d) XORP, Inc and Others/) {
    if ($2 ne $year) {
      $found_one = 1;
    }
    $ln =~ s/Copyright \(c\) (\d\d\d\d)-(\d\d\d\d) XORP, Inc and Others/Copyright \(c\) $1-$year XORP, Inc and Others/g;
  }
  elsif ($ln =~ /Copyright \(c\) (\d\d\d\d)-(\d\d\d\d) Mass/) {
    $next_xcr = 1;
  }
  elsif ($ln =~ /Copyright \(c\) (\d\d\d\d)/) {
    if ($1 ne $year) {
      $found_one = 1;
    }
    $ln =~ s/Copyright \(c\) (\d\d\d\d)/Copyright \(c\) $1-$year XORP, Inc and Others/g;
  }
  else {
    if ($next_xcr == 1) {
      $found_one = 1;
      # Need to insert a copyright line for xorp
      if ($fname =~ /\.c$/) {
	print OUTF "/* Copyright (c) $year-$year XORP, Inc and Others */\n";
      }
      else {
	print OUTF "// Copyright (c) $year-$year XORP, Inc and Others\n";
      }
      $next_xcr = 0;
    }
  }
  print OUTF $ln;
}

if ($found_one) {
  print "Updated file: $fname\n";
  `mv $tmpfname $fname`;
}
else {
  print "No changes to file: $fname\n";
}
