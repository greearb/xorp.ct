## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/Makefile.am,v 1.67 2002/12/09 09:43:53 pavlin Exp $
##

# -- Subdirectories
SUBDIRS = . harness

# -- Real Programs
bin_PROGRAMS	 = bgp

# -- Test Programs
bin_PROGRAMS	+= packet_test plumbing_test update_test

TESTS		 = aspath_test
TESTS		+= packet_coding_test
TESTS		+= test_ribin
TESTS		+= test_deletion
TESTS		+= test_filter
TESTS		+= test_cache
TESTS		+= test_nhlookup
TESTS		+= test_decision
TESTS		+= test_fanout
TESTS		+= test_dump
TESTS		+= test_ribout

noinst_PROGRAMS = $(TESTS)
noinst_LIBRARIES = libbgp.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/xrl/targets/libbgpbase.a
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
bgp_SOURCES			 = bgp.cc
bgp_SOURCES			+= main.hh
bgp_SOURCES			+= main.cc

packet_test_SOURCES		 = packet_test.hh packet_test.cc

plumbing_test_SOURCES		 = plumbing_test.hh
plumbing_test_SOURCES		+= plumbing_test.cc
plumbing_test_SOURCES		+= main.hh
plumbing_test_SOURCES		+= main.cc
plumbing_test_DEPENDENCIES	 = libbgp.a

aspath_test_SOURCES		 = aspath_test.cc		
packet_coding_test_SOURCES	 = packet_coding_test.cc
update_test_SOURCES		 = update_test.cc

test_ribin_SOURCES		 = test_ribin.cc 	
test_ribin_SOURCES		+= route_table_debug.hh
test_ribin_SOURCES		+= route_table_debug.cc
test_ribin_SOURCES		+= dummy_main.cc

test_deletion_SOURCES		 = test_deletion.cc 	
test_deletion_SOURCES		+= route_table_debug.hh
test_deletion_SOURCES		+= route_table_debug.cc
test_deletion_SOURCES		+= dummy_main.cc

test_filter_SOURCES		 = test_filter.cc 	
test_filter_SOURCES		+= route_table_debug.hh
test_filter_SOURCES		+= route_table_debug.cc
test_filter_SOURCES		+= dummy_main.cc

test_cache_SOURCES		 = test_cache.cc 	
test_cache_SOURCES		+= route_table_debug.hh
test_cache_SOURCES		+= route_table_debug.cc
test_cache_SOURCES		+= dummy_main.cc

test_nhlookup_SOURCES		 = test_nhlookup.cc 	
test_nhlookup_SOURCES		+= route_table_debug.hh
test_nhlookup_SOURCES		+= route_table_debug.cc
test_nhlookup_SOURCES		+= dummy_main.cc

test_decision_SOURCES		 = test_decision.cc 	
test_decision_SOURCES		+= route_table_debug.hh
test_decision_SOURCES		+= route_table_debug.cc
test_decision_SOURCES		+= dummy_next_hop_resolver.hh
test_decision_SOURCES		+= dummy_next_hop_resolver.cc
test_decision_SOURCES		+= dummy_main.cc

test_fanout_SOURCES		 = test_fanout.cc 	
test_fanout_SOURCES		+= route_table_debug.hh
test_fanout_SOURCES		+= route_table_debug.cc
test_fanout_SOURCES		+= dummy_main.cc

test_dump_SOURCES		 = test_dump.cc 	
test_dump_SOURCES		+= route_table_debug.hh
test_dump_SOURCES		+= route_table_debug.cc
test_dump_SOURCES		+= dummy_next_hop_resolver.hh
test_dump_SOURCES		+= dummy_next_hop_resolver.cc
test_dump_SOURCES		+= dummy_main.cc

test_ribout_SOURCES		 = test_ribout.cc 	
test_ribout_SOURCES		+= route_table_debug.hh
test_ribout_SOURCES		+= route_table_debug.cc
test_ribout_SOURCES		+= peer_handler_debug.hh
test_ribout_SOURCES		+= peer_handler_debug.cc
test_ribout_SOURCES		+= dummy_main.cc

# -- Library Sources
libbgp_a_SOURCES  = bgp_module.h
libbgp_a_SOURCES += md5.h
libbgp_a_SOURCES += aspath.hh
libbgp_a_SOURCES += attribute_manager.hh
libbgp_a_SOURCES += bgp_trie.hh
libbgp_a_SOURCES += dump_iterators.hh
libbgp_a_SOURCES += internal_message.hh
libbgp_a_SOURCES += iptuple.hh
libbgp_a_SOURCES += local_data.hh
libbgp_a_SOURCES += net_layer_reachability.hh
libbgp_a_SOURCES += next_hop_resolver.hh
libbgp_a_SOURCES += packet.hh
libbgp_a_SOURCES += parameter.hh
libbgp_a_SOURCES += path_attribute.hh
libbgp_a_SOURCES += path_attribute_list.hh
libbgp_a_SOURCES += peer.hh
libbgp_a_SOURCES += peer_data.hh
libbgp_a_SOURCES += peer_handler.hh
libbgp_a_SOURCES += peer_list.hh
libbgp_a_SOURCES += plumbing.hh
libbgp_a_SOURCES += rib_ipc_handler.hh
libbgp_a_SOURCES += route_queue.hh
libbgp_a_SOURCES += route_table_base.hh
libbgp_a_SOURCES += route_table_cache.hh
libbgp_a_SOURCES += route_table_decision.hh
libbgp_a_SOURCES += route_table_deletion.hh
libbgp_a_SOURCES += route_table_dump.hh
libbgp_a_SOURCES += route_table_fanout.hh
libbgp_a_SOURCES += route_table_filter.hh
libbgp_a_SOURCES += route_table_nhlookup.hh
libbgp_a_SOURCES += route_table_ribin.hh
libbgp_a_SOURCES += route_table_ribout.hh
libbgp_a_SOURCES += socket.hh
libbgp_a_SOURCES += subnet_route.hh
libbgp_a_SOURCES += timer_const.hh
libbgp_a_SOURCES += update_attrib.hh
libbgp_a_SOURCES += withdrawn_route.hh
libbgp_a_SOURCES += xrl_target.hh

libbgp_a_SOURCES += md5.c
libbgp_a_SOURCES += aspath.cc
libbgp_a_SOURCES += attribute_manager.cc
libbgp_a_SOURCES += bgp_trie.cc
libbgp_a_SOURCES += dump_iterators.cc
libbgp_a_SOURCES += internal_message.cc
libbgp_a_SOURCES += iptuple.cc
libbgp_a_SOURCES += keepalive_packet.cc
libbgp_a_SOURCES += local_data.cc
libbgp_a_SOURCES += net_layer_reachability.cc
libbgp_a_SOURCES += next_hop_resolver.cc
libbgp_a_SOURCES += notification_packet.cc
libbgp_a_SOURCES += open_packet.cc
libbgp_a_SOURCES += packet.cc
libbgp_a_SOURCES += parameter.cc
libbgp_a_SOURCES += path_attribute.cc
libbgp_a_SOURCES += path_attribute_list.cc
libbgp_a_SOURCES += peer.cc
libbgp_a_SOURCES += peer_data.cc
libbgp_a_SOURCES += peer_handler.cc
libbgp_a_SOURCES += peer_list.cc
libbgp_a_SOURCES += plumbing.cc
libbgp_a_SOURCES += rib_ipc_handler.cc
libbgp_a_SOURCES += route_queue.cc
libbgp_a_SOURCES += route_table_base.cc
libbgp_a_SOURCES += route_table_cache.cc
libbgp_a_SOURCES += route_table_decision.cc
libbgp_a_SOURCES += route_table_deletion.cc
libbgp_a_SOURCES += route_table_dump.cc
libbgp_a_SOURCES += route_table_fanout.cc
libbgp_a_SOURCES += route_table_filter.cc
libbgp_a_SOURCES += route_table_nhlookup.cc
libbgp_a_SOURCES += route_table_ribin.cc
libbgp_a_SOURCES += route_table_ribout.cc
libbgp_a_SOURCES += socket.cc
libbgp_a_SOURCES += subnet_route.cc
libbgp_a_SOURCES += update_attrib.cc
libbgp_a_SOURCES += update_packet.cc
libbgp_a_SOURCES += withdrawn_route.cc
libbgp_a_SOURCES += xrl_target.cc

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
