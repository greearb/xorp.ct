## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rip/Makefile.am,v 1.11 2003/07/21 18:05:55 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
check_PROGRAMS	 = test_auth
check_PROGRAMS	+= test_packets
check_PROGRAMS	+= test_request
check_PROGRAMS	+= test_route_walk
check_PROGRAMS	+= test_timers
check_PROGRAMS	+= test_update_queue
TESTS		 = $(check_PROGRAMS)

# -- No-install Programs
noinst_PROGRAMS  = 

# -- No-install Libraries
noinst_LIBRARIES		 = librip.a

# -- Libraries and library flags to link the binary programs
LDFLAGS				 =
LDADD				 = ./librip.a
LDADD				+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_auth_SOURCES		 = test_auth.cc
test_packets_SOURCES		 = test_packets.cc
test_request_SOURCES		 = test_request.cc
test_route_walk_SOURCES		 = test_route_walk.cc
test_timers_SOURCES		 = test_timers.cc
test_update_queue_SOURCES	 = test_update_queue.cc

# -- Library Sources
librip_a_SOURCES		 = rip_module.h

librip_a_SOURCES		+= auth.hh
librip_a_SOURCES		+= constants.hh
librip_a_SOURCES		+= packets.hh
librip_a_SOURCES		+= packet_queue.hh
librip_a_SOURCES		+= peer.hh
librip_a_SOURCES		+= port.hh
librip_a_SOURCES		+= port_io.hh
librip_a_SOURCES		+= port_manager.hh
librip_a_SOURCES		+= redist.hh
librip_a_SOURCES		+= route_db.hh
librip_a_SOURCES		+= route_entry.hh
librip_a_SOURCES		+= update_queue.hh

librip_a_SOURCES		+= auth.cc
librip_a_SOURCES		+= packet_queue.cc
librip_a_SOURCES		+= peer.cc
librip_a_SOURCES		+= port.cc
librip_a_SOURCES		+= redist.cc
librip_a_SOURCES		+= route_db.cc
librip_a_SOURCES		+= route_entry.cc
librip_a_SOURCES		+= update_queue.cc

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

