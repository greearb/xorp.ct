## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/Makefile.am,v 1.34 2004/11/12 02:44:27 bms Exp $
##

# -- Subdirectories
SUBDIRS = . tools harness

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_bgp

# -- Test Programs
TESTS		 = test_packet
#TESTS		+= test_plumbing
#TESTS		+= update_test
TESTS		+= aspath_test
TESTS		+= test_packet_coding
TESTS		+= isolation_tests
TESTS		+= test_peer_data
TESTS		+= test_leaks.sh

# -- No-install Programs
check_PROGRAMS	 = test_packet
check_PROGRAMS	+= test_plumbing
check_PROGRAMS	+= update_test
check_PROGRAMS	+= aspath_test
check_PROGRAMS	+= test_packet_coding
check_PROGRAMS	+= isolation_tests
check_PROGRAMS	+= test_peer_data

# -- No-install Libtool Libraries
noinst_LTLIBRARIES	 = libbgp.la

# -- Libraries and library flags to link the binary programs
LDADD		 = libbgp.la
LDADD		+= $(top_builddir)/xrl/targets/libbgpbase.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libbgpmibtrapsxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_bgp_SOURCES		 = bgp.cc
xorp_bgp_SOURCES		+= bgp.hh
xorp_bgp_SOURCES		+= main.cc

test_packet_SOURCES		 = test_test.hh test_packet.cc

test_plumbing_SOURCES		 = test_plumbing.hh
test_plumbing_SOURCES		+= test_plumbing.cc
test_plumbing_SOURCES		+= bgp.hh
test_plumbing_SOURCES		+= bgp.cc
test_plumbing_DEPENDENCIES	 = libbgp.la

aspath_test_SOURCES		 = aspath_test.cc
test_packet_coding_SOURCES	 = test_packet_coding.cc
update_test_SOURCES		 = update_test.cc

isolation_tests_SOURCES		= test_main.cc
isolation_tests_SOURCES		+= test_ribin.cc
isolation_tests_SOURCES		+= test_deletion.cc
isolation_tests_SOURCES		+= test_filter.cc
isolation_tests_SOURCES		+= test_cache.cc
isolation_tests_SOURCES		+= test_nhlookup.cc
isolation_tests_SOURCES		+= test_decision.cc
isolation_tests_SOURCES		+= test_fanout.cc
isolation_tests_SOURCES		+= test_dump.cc
isolation_tests_SOURCES		+= test_ribout.cc
isolation_tests_SOURCES		+= test_next_hop_resolver.cc
isolation_tests_SOURCES		+= route_table_debug.hh
isolation_tests_SOURCES		+= route_table_debug.cc
isolation_tests_SOURCES		+= dummy_main.cc
isolation_tests_SOURCES		+= dummy_next_hop_resolver.cc
isolation_tests_SOURCES		+= dummy_next_hop_resolver.hh
isolation_tests_SOURCES		+= peer_handler_debug.hh
isolation_tests_SOURCES		+= peer_handler_debug.cc
isolation_tests_SOURCES		+= test_subnet_route.cc

test_peer_data_SOURCES		= test_peer_data.cc

# -- Library Sources
libbgp_la_SOURCES  = bgp_module.h
libbgp_la_SOURCES += aspath.hh
libbgp_la_SOURCES += attribute_manager.hh
libbgp_la_SOURCES += bgp_trie.hh
libbgp_la_SOURCES += bgp_varrw.hh
libbgp_la_SOURCES += dump_iterators.hh
libbgp_la_SOURCES += exceptions.hh
libbgp_la_SOURCES += internal_message.hh
libbgp_la_SOURCES += iptuple.hh
libbgp_la_SOURCES += local_data.hh
libbgp_la_SOURCES += next_hop_resolver.hh
libbgp_la_SOURCES += packet.hh
libbgp_la_SOURCES += parameter.hh
libbgp_la_SOURCES += path_attribute.hh
libbgp_la_SOURCES += peer.hh
libbgp_la_SOURCES += peer_data.hh
libbgp_la_SOURCES += peer_handler.hh
libbgp_la_SOURCES += peer_list.hh
libbgp_la_SOURCES += plumbing.hh
libbgp_la_SOURCES += process_watch.hh
libbgp_la_SOURCES += profile_vars.hh
libbgp_la_SOURCES += rib_ipc_handler.hh
libbgp_la_SOURCES += route_queue.hh
libbgp_la_SOURCES += route_table_base.hh
libbgp_la_SOURCES += route_table_cache.hh
libbgp_la_SOURCES += route_table_decision.hh
libbgp_la_SOURCES += route_table_deletion.hh
libbgp_la_SOURCES += route_table_dump.hh
libbgp_la_SOURCES += route_table_fanout.hh
libbgp_la_SOURCES += route_table_filter.hh
libbgp_la_SOURCES += route_table_nhlookup.hh
libbgp_la_SOURCES += route_table_policy.hh
libbgp_la_SOURCES += route_table_policy_im.hh
libbgp_la_SOURCES += route_table_policy_sm.hh
libbgp_la_SOURCES += route_table_ribin.hh
libbgp_la_SOURCES += route_table_ribout.hh
libbgp_la_SOURCES += route_table_reader.hh
libbgp_la_SOURCES += socket.hh
libbgp_la_SOURCES += subnet_route.hh
libbgp_la_SOURCES += timer_const.hh
libbgp_la_SOURCES += update_attrib.hh
libbgp_la_SOURCES += xrl_target.hh

libbgp_la_SOURCES += aspath.cc
libbgp_la_SOURCES += attribute_manager.cc
libbgp_la_SOURCES += bgp_trie.cc
libbgp_la_SOURCES += bgp_varrw.cc
libbgp_la_SOURCES += dump_iterators.cc
libbgp_la_SOURCES += internal_message.cc
libbgp_la_SOURCES += iptuple.cc
libbgp_la_SOURCES += local_data.cc
libbgp_la_SOURCES += next_hop_resolver.cc
libbgp_la_SOURCES += notification_packet.cc
libbgp_la_SOURCES += open_packet.cc
libbgp_la_SOURCES += packet.cc
libbgp_la_SOURCES += parameter.cc
libbgp_la_SOURCES += path_attribute.cc
libbgp_la_SOURCES += peer.cc
libbgp_la_SOURCES += peer_data.cc
libbgp_la_SOURCES += peer_handler.cc
libbgp_la_SOURCES += peer_list.cc
libbgp_la_SOURCES += plumbing.cc
libbgp_la_SOURCES += process_watch.cc
libbgp_la_SOURCES += profile_vars.cc
libbgp_la_SOURCES += rib_ipc_handler.cc
libbgp_la_SOURCES += route_queue.cc
libbgp_la_SOURCES += route_table_base.cc
libbgp_la_SOURCES += route_table_cache.cc
libbgp_la_SOURCES += route_table_decision.cc
libbgp_la_SOURCES += route_table_deletion.cc
libbgp_la_SOURCES += route_table_dump.cc
libbgp_la_SOURCES += route_table_fanout.cc
libbgp_la_SOURCES += route_table_filter.cc
libbgp_la_SOURCES += route_table_nhlookup.cc
libbgp_la_SOURCES += route_table_policy.cc
libbgp_la_SOURCES += route_table_policy_im.cc
libbgp_la_SOURCES += route_table_policy_sm.cc
libbgp_la_SOURCES += route_table_ribin.cc
libbgp_la_SOURCES += route_table_ribout.cc
libbgp_la_SOURCES += route_table_reader.cc
libbgp_la_SOURCES += socket.cc
libbgp_la_SOURCES += subnet_route.cc
libbgp_la_SOURCES += update_attrib.cc
libbgp_la_SOURCES += update_packet.cc
libbgp_la_SOURCES += xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = BUGS
EXTRA_DIST	+= configs
EXTRA_DIST	+= configure_bgp.sh
EXTRA_DIST	+= docs
EXTRA_DIST	+= run_tests
EXTRA_DIST	+= test_cache.reference
EXTRA_DIST	+= test_decision.reference
EXTRA_DIST	+= test_deletion.reference
EXTRA_DIST	+= test_dump.reference
EXTRA_DIST	+= test_fanout.reference
EXTRA_DIST	+= test_filter.reference
EXTRA_DIST	+= test_nhlookup.reference
EXTRA_DIST	+= test_ribin.reference
EXTRA_DIST	+= test_ribout.reference
EXTRA_DIST	+= xrl_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

