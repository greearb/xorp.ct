## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rip/Makefile.am,v 1.21 2004/02/20 01:22:03 hodson Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_rip xorp_ripng

# -- Test Programs
TESTS		 = test_auth
TESTS		+= test_packets
TESTS		+= test_request
TESTS		+= test_route_walk
TESTS		+= test_timers
TESTS		+= test_update_queue
TESTS		+= test_outputs

# -- No-install Programs
noinst_PROGRAMS	 = test_auth
noinst_PROGRAMS	+= test_packets
noinst_PROGRAMS	+= test_request
noinst_PROGRAMS	+= test_route_walk
noinst_PROGRAMS	+= test_timers
noinst_PROGRAMS	+= test_update_queue
noinst_PROGRAMS	+= test_outputs

# -- No-install Libraries
noinst_LIBRARIES  = librip.a
noinst_LIBRARIES += libripng.a
noinst_LIBRARIES += libripxrl.a
noinst_LIBRARIES += libripngxrl.a

# -- Libraries and library flags to link the binary programs
LDFLAGS				 =
#LDADD				 = ./librip.a
LDADD				 = $(top_builddir)/libxorp/libxorp.a

# -- Real Program Sources

xorp_rip_SOURCES		 = xorp_rip.cc
xorp_rip_SOURCES		+= xrl_target_common.hh
xorp_rip_SOURCES		+= xrl_target_rip.hh
xorp_rip_SOURCES		+= xrl_target_rip.cc
xorp_rip_LDADD			 = ./libripxrl.a ./librip.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/targets/libripbase.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libribxif.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
xorp_rip_LDADD			+= $(top_builddir)/libfeaclient/libfeaclient.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libsocket4locatorxif.a
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libsocket4xif.a
xorp_rip_LDADD			+= $(top_builddir)/libxipc/libxipc.a
xorp_rip_LDADD			+= $(top_builddir)/libcomm/libcomm.a
xorp_rip_LDADD			+= $(top_builddir)/libxorp/libxorp.a

xorp_ripng_SOURCES		 = xorp_ripng.cc
xorp_ripng_SOURCES		+= xrl_target_common.hh
xorp_ripng_SOURCES		+= xrl_target_ripng.hh
xorp_ripng_SOURCES		+= xrl_target_ripng.cc
xorp_ripng_LDADD		 = ./libripngxrl.a ./libripng.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/targets/libripngbase.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
xorp_ripng_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6locatorxif.a
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6xif.a
xorp_ripng_LDADD		+= $(top_builddir)/libxipc/libxipc.a
xorp_ripng_LDADD		+= $(top_builddir)/libcomm/libcomm.a
xorp_ripng_LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Test Program Sources
test_auth_SOURCES		 = test_auth.cc
test_auth_LDADD			 = ./librip.a $(LDADD)

test_packets_SOURCES		 = test_packets.cc
test_packets_LDADD		 = ./librip.a $(LDADD)

test_request_SOURCES		 = test_request.cc
test_request_LDADD		 = ./librip.a $(LDADD)

test_route_walk_SOURCES		 = test_route_walk.cc
test_route_walk_LDADD		 = ./librip.a $(LDADD)

test_timers_SOURCES		 = test_timers.cc
test_timers_LDADD		 = ./librip.a $(LDADD)

test_update_queue_SOURCES	 = test_update_queue.cc
test_update_queue_LDADD		 = ./librip.a $(LDADD)

test_outputs_SOURCES		 = test_outputs.cc
test_outputs_LDADD		 = ./librip.a libripng.a $(LDADD)

# -- RIP Core Library Sources
librip_a_CXXFLAGS		 = -DINSTANTIATE_IPV4
librip_a_SOURCES		 = rip_module.h

librip_a_SOURCES		+= auth.hh
librip_a_SOURCES		+= constants.hh
librip_a_SOURCES		+= output.hh
librip_a_SOURCES		+= output_table.hh
librip_a_SOURCES		+= output_updates.hh
librip_a_SOURCES		+= packet_assembly.hh
librip_a_SOURCES		+= packet_queue.hh
librip_a_SOURCES		+= packets.hh
librip_a_SOURCES		+= peer.hh
librip_a_SOURCES		+= port.hh
librip_a_SOURCES		+= port_io.hh
librip_a_SOURCES		+= port_manager.hh
librip_a_SOURCES		+= port_vars.hh
librip_a_SOURCES		+= redist.hh
librip_a_SOURCES		+= rib_notifier_base.hh
librip_a_SOURCES		+= route_db.hh
librip_a_SOURCES		+= route_entry.hh
librip_a_SOURCES		+= system.hh
librip_a_SOURCES		+= update_queue.hh

librip_a_SOURCES		+= auth.cc
librip_a_SOURCES		+= output_table.cc
librip_a_SOURCES		+= output_updates.cc
librip_a_SOURCES		+= packet_queue.cc
librip_a_SOURCES		+= peer.cc
librip_a_SOURCES		+= port.cc
librip_a_SOURCES		+= redist.cc
librip_a_SOURCES		+= rib_notifier_base.cc
librip_a_SOURCES		+= route_db.cc
librip_a_SOURCES		+= route_entry.cc
librip_a_SOURCES		+= update_queue.cc

# -- RIP NG Core Library Sources
libripng_a_CXXFLAGS		 = -DINSTANTIATE_IPV6
libripng_a_SOURCES		 = rip_module.h

libripng_a_SOURCES		+= constants.hh
libripng_a_SOURCES		+= output.hh
libripng_a_SOURCES		+= output_table.hh
libripng_a_SOURCES		+= output_updates.hh
libripng_a_SOURCES		+= packet_assembly.hh
libripng_a_SOURCES		+= packet_queue.hh
libripng_a_SOURCES		+= packets.hh
libripng_a_SOURCES		+= peer.hh
libripng_a_SOURCES		+= port.hh
libripng_a_SOURCES		+= port_io.hh
libripng_a_SOURCES		+= port_manager.hh
libripng_a_SOURCES		+= port_vars.hh
libripng_a_SOURCES		+= redist.hh
libripng_a_SOURCES		+= rib_notifier_base.hh
libripng_a_SOURCES		+= route_db.hh
libripng_a_SOURCES		+= route_entry.hh
libripng_a_SOURCES		+= system.hh
libripng_a_SOURCES		+= update_queue.hh

libripng_a_SOURCES		+= output_table.cc
libripng_a_SOURCES		+= output_updates.cc
libripng_a_SOURCES		+= packet_queue.cc
libripng_a_SOURCES		+= peer.cc
libripng_a_SOURCES		+= port.cc
libripng_a_SOURCES		+= redist.cc
libripng_a_SOURCES		+= rib_notifier_base.cc
libripng_a_SOURCES		+= route_db.cc
libripng_a_SOURCES		+= route_entry.cc
libripng_a_SOURCES		+= update_queue.cc

# -- RIP Xrl library sources
libripxrl_a_CXXFLAGS		 = -DINSTANTIATE_IPV4
libripxrl_a_SOURCES		 = xrl_port_manager.hh
libripxrl_a_SOURCES		+= xrl_port_io.hh
libripxrl_a_SOURCES		+= xrl_process_spy.hh
libripxrl_a_SOURCES		+= xrl_rib_notifier.hh

libripxrl_a_SOURCES		+= xrl_port_manager.cc
libripxrl_a_SOURCES		+= xrl_port_io.cc
libripxrl_a_SOURCES		+= xrl_process_spy.cc
libripxrl_a_SOURCES		+= xrl_rib_notifier.cc

# -- RIP NG Xrl library sources
libripngxrl_a_CXXFLAGS		 = -DINSTANTIATE_IPV6
libripngxrl_a_SOURCES		 = xrl_port_manager.hh
libripngxrl_a_SOURCES		+= xrl_port_io.hh
libripngxrl_a_SOURCES		+= xrl_process_spy.hh
libripngxrl_a_SOURCES		+= xrl_rib_notifier.hh

libripngxrl_a_SOURCES		+= xrl_port_manager.cc
libripngxrl_a_SOURCES		+= xrl_port_io.cc
libripngxrl_a_SOURCES		+= xrl_process_spy.cc
libripngxrl_a_SOURCES		+= xrl_rib_notifier.cc

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

