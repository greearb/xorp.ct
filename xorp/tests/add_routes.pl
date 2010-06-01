#!/usr/bin/perl

#  Add a bunch of routes to the specified routing table
#  Used to test fib2mrib propagation.

use strict;

my $amt = 2000;
my $a = 1;
my $b = 1;
my $c = 10;
my $d = 0;
my $sigb = 24;
my $dev = "rddVR0";
my $table = 10001;
my $gw = "1.1.1.2";

my $i = 0;
for ($i = 0; $i<$amt; $i++) {
  my $cmd = "ip route add $a.$b.$c.$d/$sigb via $gw dev $dev table $table";
  print "$cmd\n";
  `$cmd`;
  $c++;
  if ($c >= 255) {
    $b++;
    $c = 1;
    if ($b >= 255) {
      $a++;
      $b = 1;
    }
  }
}
