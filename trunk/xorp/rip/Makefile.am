## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rip/Makefile.am,v 1.32 2004/09/17 23:56:16 pavlin Exp $
##

SUBDIRS 			 = . tools

# Redefine the bin installation directory
bindir				 = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS			 = xorp_rip xorp_ripng

# -- Test Programs
TESTS				 = test_auth
TESTS				+= test_packets
TESTS				+= test_request
TESTS				+= test_route_walk
TESTS				+= test_timers
TESTS				+= test_update_queue
TESTS				+= test_outputs

# -- No-install Programs
noinst_PROGRAMS	 		 = test_auth
noinst_PROGRAMS			+= test_packets
noinst_PROGRAMS			+= test_request
noinst_PROGRAMS			+= test_route_walk
noinst_PROGRAMS			+= test_timers
noinst_PROGRAMS			+= test_update_queue
noinst_PROGRAMS			+= test_outputs

# -- No-install Libtool Libraries
noinst_LTLIBRARIES		 = librip.la
noinst_LTLIBRARIES		+= libripng.la
noinst_LTLIBRARIES		+= libripxrl.la
noinst_LTLIBRARIES		+= libripngxrl.la

# -- Libraries and library flags to link the binary programs
LDFLAGS				 =
LDADD				 = $(top_builddir)/libxorp/libxorp.la

# -- Real Program Sources
xorp_rip_CXXFLAGS		 = -DINSTANTIATE_IPV4
xorp_rip_SOURCES		 = xorp_rip_main.cc
xorp_rip_SOURCES		+= xrl_target_common.hh
xorp_rip_SOURCES		+= xrl_target_rip.hh
xorp_rip_SOURCES		+= xrl_target_rip.cc
xorp_rip_LDADD			 = ./libripxrl.la ./librip.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/targets/libripbase.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libribxif.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
xorp_rip_LDADD			+= $(top_builddir)/libfeaclient/libfeaclient.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libsocket4locatorxif.la
xorp_rip_LDADD			+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
xorp_rip_LDADD			+= $(top_builddir)/policy/backend/libpolicybackend.la
xorp_rip_LDADD			+= $(top_builddir)/libxipc/libxipc.la
xorp_rip_LDADD			+= $(top_builddir)/libcomm/libcomm.la
xorp_rip_LDADD			+= $(top_builddir)/libxorp/libxorp.la

xorp_ripng_CXXFLAGS		 = -DINSTANTIATE_IPV6
xorp_ripng_SOURCES		 = xorp_rip_main.cc
xorp_ripng_SOURCES		+= xrl_target_common.hh
xorp_ripng_SOURCES		+= xrl_target_ripng.hh
xorp_ripng_SOURCES		+= xrl_target_ripng.cc
xorp_ripng_LDADD		 = ./libripngxrl.la ./libripng.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/targets/libripngbase.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
xorp_ripng_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6locatorxif.la
xorp_ripng_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
xorp_ripng_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
xorp_ripng_LDADD		+= $(top_builddir)/libxipc/libxipc.la
xorp_ripng_LDADD		+= $(top_builddir)/libcomm/libcomm.la
xorp_ripng_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Test Program Sources
test_auth_SOURCES		 = test_auth.cc test_utils.hh
test_auth_LDADD			 = ./librip.la $(LDADD)

test_packets_SOURCES		 = test_packets.cc test_utils.hh
test_packets_LDADD		 = ./librip.la $(LDADD)

test_request_SOURCES		 = test_request.cc test_utils.hh
test_request_LDADD		 = ./librip.la
test_request_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_request_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_request_LDADD		+= $(LDADD)

test_route_walk_SOURCES		 = test_route_walk.cc test_utils.hh
test_route_walk_LDADD		 = ./librip.la ./libripng.la
test_route_walk_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_route_walk_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_route_walk_LDADD		+= $(LDADD)

test_timers_SOURCES		 = test_timers.cc test_utils.hh
test_timers_LDADD		 = ./librip.la ./libripng.la
test_timers_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_timers_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_timers_LDADD		+= $(LDADD)

test_update_queue_SOURCES	 = test_update_queue.cc test_utils.hh
test_update_queue_LDADD		 = ./librip.la ./libripng.la
test_update_queue_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_update_queue_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_update_queue_LDADD		+= $(LDADD)

test_outputs_SOURCES		 = test_outputs.cc test_utils.hh
test_outputs_LDADD		 = ./librip.la ./libripng.la
test_outputs_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_outputs_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_outputs_LDADD		+= $(LDADD)


# -- RIP Core Library Sources
librip_la_CXXFLAGS		 = -DINSTANTIATE_IPV4
librip_la_SOURCES		 = rip_module.h

librip_la_SOURCES		+= auth.hh
librip_la_SOURCES		+= constants.hh
librip_la_SOURCES		+= output.hh
librip_la_SOURCES		+= output_table.hh
librip_la_SOURCES		+= output_updates.hh
librip_la_SOURCES		+= packet_assembly.hh
librip_la_SOURCES		+= packet_queue.hh
librip_la_SOURCES		+= packets.hh
librip_la_SOURCES		+= peer.hh
librip_la_SOURCES		+= port.hh
librip_la_SOURCES		+= port_io.hh
librip_la_SOURCES		+= port_manager.hh
librip_la_SOURCES		+= port_vars.hh
librip_la_SOURCES		+= redist.hh
librip_la_SOURCES		+= rib_notifier_base.hh
librip_la_SOURCES		+= rip_varrw.hh
librip_la_SOURCES		+= route_db.hh
librip_la_SOURCES		+= route_entry.hh
librip_la_SOURCES		+= system.hh
librip_la_SOURCES		+= update_queue.hh

librip_la_SOURCES		+= auth.cc
librip_la_SOURCES		+= output_table.cc
librip_la_SOURCES		+= output_updates.cc
librip_la_SOURCES		+= packet_queue.cc
librip_la_SOURCES		+= peer.cc
librip_la_SOURCES		+= port.cc
librip_la_SOURCES		+= redist.cc
librip_la_SOURCES		+= rib_notifier_base.cc
librip_la_SOURCES		+= rip_varrw.cc
librip_la_SOURCES		+= route_db.cc
librip_la_SOURCES		+= route_entry.cc
librip_la_SOURCES		+= update_queue.cc

# -- RIP NG Core Library Sources
libripng_la_CXXFLAGS		 = -DINSTANTIATE_IPV6
libripng_la_SOURCES		 = rip_module.h

libripng_la_SOURCES		+= constants.hh
libripng_la_SOURCES		+= output.hh
libripng_la_SOURCES		+= output_table.hh
libripng_la_SOURCES		+= output_updates.hh
libripng_la_SOURCES		+= packet_assembly.hh
libripng_la_SOURCES		+= packet_queue.hh
libripng_la_SOURCES		+= packets.hh
libripng_la_SOURCES		+= peer.hh
libripng_la_SOURCES		+= port.hh
libripng_la_SOURCES		+= port_io.hh
libripng_la_SOURCES		+= port_manager.hh
libripng_la_SOURCES		+= port_vars.hh
libripng_la_SOURCES		+= redist.hh
libripng_la_SOURCES		+= rib_notifier_base.hh
libripng_la_SOURCES		+= rip_varrw.hh
libripng_la_SOURCES		+= route_db.hh
libripng_la_SOURCES		+= route_entry.hh
libripng_la_SOURCES		+= system.hh
libripng_la_SOURCES		+= update_queue.hh

libripng_la_SOURCES		+= output_table.cc
libripng_la_SOURCES		+= output_updates.cc
libripng_la_SOURCES		+= packet_queue.cc
libripng_la_SOURCES		+= peer.cc
libripng_la_SOURCES		+= port.cc
libripng_la_SOURCES		+= redist.cc
libripng_la_SOURCES		+= rib_notifier_base.cc
libripng_la_SOURCES		+= rip_varrw.cc
libripng_la_SOURCES		+= route_db.cc
libripng_la_SOURCES		+= route_entry.cc
libripng_la_SOURCES		+= update_queue.cc

# -- RIP XRL library sources
libripxrl_la_CXXFLAGS		 = -DINSTANTIATE_IPV4
libripxrl_la_SOURCES		 = xrl_config.hh
libripxrl_la_SOURCES		+= xrl_port_manager.hh
libripxrl_la_SOURCES		+= xrl_port_io.hh
libripxrl_la_SOURCES		+= xrl_process_spy.hh
libripxrl_la_SOURCES		+= xrl_redist_manager.hh
libripxrl_la_SOURCES		+= xrl_rib_notifier.hh

libripxrl_la_SOURCES		+= xrl_config.cc
libripxrl_la_SOURCES		+= xrl_port_manager.cc
libripxrl_la_SOURCES		+= xrl_port_io.cc
libripxrl_la_SOURCES		+= xrl_process_spy.cc
libripxrl_la_SOURCES		+= xrl_redist_manager.cc
libripxrl_la_SOURCES		+= xrl_rib_notifier.cc

# -- RIP NG Xrl library sources
libripngxrl_la_CXXFLAGS		 = -DINSTANTIATE_IPV6
libripngxrl_la_SOURCES		 = xrl_config.hh
libripngxrl_la_SOURCES		+= xrl_port_manager.hh
libripngxrl_la_SOURCES		+= xrl_port_io.hh
libripngxrl_la_SOURCES		+= xrl_process_spy.hh
libripngxrl_la_SOURCES		+= xrl_redist_manager.hh
libripngxrl_la_SOURCES		+= xrl_rib_notifier.hh

libripngxrl_la_SOURCES		+= xrl_config.cc
libripngxrl_la_SOURCES		+= xrl_port_manager.cc
libripngxrl_la_SOURCES		+= xrl_port_io.cc
libripngxrl_la_SOURCES		+= xrl_process_spy.cc
libripngxrl_la_SOURCES		+= xrl_rib_notifier.cc
libripngxrl_la_SOURCES		+= xrl_redist_manager.cc

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

