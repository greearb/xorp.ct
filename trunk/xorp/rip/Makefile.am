## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rip/Makefile.am,v 1.7 2003/05/07 21:58:12 pavlin Exp $
##

noinst_LIBRARIES	 = librip.a
LDFLAGS			 =
LDADD			 = ./librip.a
LDADD			+= $(top_builddir)/libxorp/libxorp.a

noinst_PROGRAMS		 = $(TESTS)
TESTS			 = test_auth
TESTS			+= test_packets
TESTS			+= test_timers

test_auth_SOURCES	 = test_auth.cc
test_packets_SOURCES	 = test_packets.cc
test_timers_SOURCES	 = test_timers.cc

librip_a_SOURCES	 = rip_module.h

librip_a_SOURCES	+= auth.hh
librip_a_SOURCES	+= constants.hh
librip_a_SOURCES	+= packets.hh
librip_a_SOURCES	+= peer.hh
librip_a_SOURCES	+= port.hh
librip_a_SOURCES	+= port_io.hh
librip_a_SOURCES	+= port_manager.hh
librip_a_SOURCES	+= redist.hh
librip_a_SOURCES	+= route_db.hh
librip_a_SOURCES	+= route_entry.hh
librip_a_SOURCES	+= update_queue.hh

librip_a_SOURCES	+= auth.cc
librip_a_SOURCES	+= peer.cc
librip_a_SOURCES	+= port.cc
librip_a_SOURCES	+= redist.cc
librip_a_SOURCES	+= route_db.cc
librip_a_SOURCES	+= route_entry.cc
librip_a_SOURCES	+= update_queue.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

