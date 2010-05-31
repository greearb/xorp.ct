#!/usr/bin/perl

#  This uses call_xrl to load a bunch of 'originated' routes into
#  the current active BGP router process (possibly discriminated by
#  XORP_FINDER_SERVER_PORT environment variable if you are running
#  multiple XORP instances.
#
#  Used for load-testing bgp

use strict;

my $usage = "
This tool adds 'originated' routes to bgp.  Used primarily
for load testing bgp.

ip=a.b.c.d     # Starting IP
nh=a.b.c.d     # Next-hop IP
sigb=x         # Scope (ie, 24 for a 255.255.255.0 network)
ucast=[0|1]    # Are these unicast (normal) routes?
mcast=[0|1]    # Are these multicast routes?
amt=x          # How many routes to add?

Example: originate_routes.pl ip=10.1.1.0 nh=10.1.1.1 sigb=4 amt=500
";

my $start_ip_a = 192;
my $start_ip_b = 168;
my $start_ip_c = 101;
my $start_ip_d = 0;
my $nh_ip_a = 192;
my $nh_ip_b = 168;
my $nh_ip_c = 101;
my $nh_ip_d = 1;
my $sigb = 30;
my $ucast = 1;
my $mcast = 0;
my $amt = 1000;

my $i;
for ($i = 0; $i<@ARGV; $i++) {
  my $var = $ARGV[$i];
  if ($var =~ m/(\S+)=(.*)/) {
    my $arg = $1;
    my $val = $2;
    handleCmdLineArg($arg, $val);
  }
  elsif (($var eq "-h") || ($var eq "-?") || ($var eq "--help")) {
    print "Usage: $usage\n";
    exit 0;
  }
  else {
    print "Cmd-Line arg -:$var:- not supported!\n$usage";
    exit 1;
  }
}

my $ip_incr = (1<<(32 - $sigb));

print "Amt: $amt  ip_incr: $ip_incr  sigb: $sigb\n";

for ($i = 0; $i<$amt; $i++) {
  my $nlri = "$start_ip_a.$start_ip_b.$start_ip_c.$start_ip_d/$sigb";
  my $nh = "$nh_ip_a.$nh_ip_b.$nh_ip_c.$nh_ip_d";
  my $cmd = "call_xrl finder://bgp/bgp/0.3/originate_route4?nlri:ipv4net=$nlri\\\&next_hop:ipv4=$nh\\\&unicast:bool=$ucast\\\&multicast:bool=$mcast";
  #print "cmd -:$cmd:-";
  #exit 0;
  if (system("$cmd") != 0) {
    print "ERROR, command failed: $cmd";
    print " Completed $i commands\n";
    exit 1;
  }

  if (($i % 499) == 0) {
    print "Created: $i routes so far.\n";
  }

  # Increment IP addresses
  ($start_ip_a, $start_ip_b, $start_ip_c, $start_ip_d) = incr_ip($start_ip_a, $start_ip_b, $start_ip_c, $start_ip_d);
  ($nh_ip_a, $nh_ip_b, $nh_ip_c, $nh_ip_d) = incr_ip($nh_ip_a, $nh_ip_b, $nh_ip_c, $nh_ip_d);
}

exit 0;

sub incr_ip {
  my $a = shift;
  my $b = shift;
  my $c = shift;
  my $d = shift;

  my $v = ($a << 24) | ($b << 16) | ($c << 8) | $d;
  $v += $ip_incr;
  return (($v >> 24) & 0xff, ($v >> 16) & 0xff, ($v >> 8) & 0xff, $v & 0xff);
}

sub handleCmdLineArg {
  my $arg = $_[0];
  my $val = $_[1];

  if ($arg eq "ip") {
    ($start_ip_a, $start_ip_b, $start_ip_c, $start_ip_d) = split(/\./, $val);
  }
  elsif ($arg eq "nh") {
    ($nh_ip_a, $nh_ip_b, $nh_ip_c, $nh_ip_d) = split(/\./, $val);
  }
  elsif ($arg eq "sigb") {
    $sigb = $val;
  }
  elsif ($arg eq "ucast") {
    $ucast = $val;
  }
  elsif ($arg eq "mcast") {
    $mcast = $val;
  }
  elsif ($arg eq "amt") {
    $amt = $val;
  }
  # Add support for more cmd line args here.
  else {
    print "Cmd-Line arg -:$arg:- not supported!\n$usage";
    exit 1;
  }
}
