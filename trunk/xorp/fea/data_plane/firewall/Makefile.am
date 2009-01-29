## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/data_plane/firewall/Makefile.am,v 1.1 2008/04/26 00:59:46 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Libraries
lib_LTLIBRARIES = libxorp_firewall.la

# -- Library Sources
libxorp_firewall_la_SOURCES	 = firewall_get_dummy.hh
libxorp_firewall_la_SOURCES	+= firewall_get_ipfw2.hh
libxorp_firewall_la_SOURCES	+= firewall_get_netfilter.hh
libxorp_firewall_la_SOURCES	+= firewall_get_pf.hh
libxorp_firewall_la_SOURCES	+= firewall_set_dummy.hh
libxorp_firewall_la_SOURCES	+= firewall_set_ipfw2.hh
libxorp_firewall_la_SOURCES	+= firewall_set_netfilter.hh
libxorp_firewall_la_SOURCES	+= firewall_set_pf.hh

libxorp_firewall_la_SOURCES	+= firewall_get_dummy.cc
libxorp_firewall_la_SOURCES	+= firewall_get_ipfw2.cc
libxorp_firewall_la_SOURCES	+= firewall_get_netfilter.cc
libxorp_firewall_la_SOURCES	+= firewall_get_pf.cc
libxorp_firewall_la_SOURCES	+= firewall_set_dummy.cc
libxorp_firewall_la_SOURCES	+= firewall_set_ipfw2.cc
libxorp_firewall_la_SOURCES	+= firewall_set_netfilter.cc
libxorp_firewall_la_SOURCES	+= firewall_set_pf.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
