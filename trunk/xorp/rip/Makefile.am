## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rip/Makefile.am,v 1.3 2003/04/10 00:27:42 hodson Exp $
##

noinst_LIBRARIES	 = librip.a
LDFLAGS			 =
LDADD			 = ./librip.a
LDADD			+= $(top_builddir)/libxorp/libxorp.a

noinst_PROGRAMS		 = $(TESTS)
TESTS			 = test_packets
TESTS			+= test_timers_update

test_packets_SOURCES	 = test_packets.cc
test_timers_update_SOURCES = test_timers_update.cc

librip_a_SOURCES	 = rip_module.h

librip_a_SOURCES	+= constants.hh
librip_a_SOURCES	+= packets.hh
librip_a_SOURCES	+= peer.hh
librip_a_SOURCES	+= port.hh
librip_a_SOURCES	+= port_io.hh
librip_a_SOURCES	+= port_manager.hh
librip_a_SOURCES	+= route_db.hh
librip_a_SOURCES	+= route_entry.hh
librip_a_SOURCES	+= update_queue.hh

librip_a_SOURCES	+= port.cc
librip_a_SOURCES	+= route_db.cc
librip_a_SOURCES	+= route_entry.cc
librip_a_SOURCES	+= update_queue.cc
