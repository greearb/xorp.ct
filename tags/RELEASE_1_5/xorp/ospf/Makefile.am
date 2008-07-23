## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/ospf/Makefile.am,v 1.27 2007/10/04 20:46:16 pavlin Exp $
##

# -- Subdirectories
SUBDIRS = . tools

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_ospfv2 xorp_ospfv3

# -- Test Programs
check_PROGRAMS	 = test_packet
check_PROGRAMS	+= test_checksum
check_PROGRAMS	+= test_peering
check_PROGRAMS	+= test_routing
check_PROGRAMS	+= test_routing_database
check_PROGRAMS	+= test_routing_table
check_PROGRAMS	+= test_build_lsa_main
check_PROGRAMS	+= test_routing_interactive

TESTS		 = test_packet$(EXEEXT)
TESTS		+= test_checksum$(EXEEXT)
TESTS		+= test_peering$(EXEEXT)
TESTS		+= test_routing$(EXEEXT)
TESTS		+= test_routing_table$(EXEEXT)
TESTS		+= test_routing1.py

# -- No-install Libtool Libraries
noinst_LTLIBRARIES	 = libospf.la

# -- Libraries and library flags to link the binary programs
LDADD		 = libospf.la
LDADD		+= $(top_builddir)/xrl/targets/libospfv2base.la
LDADD		+= $(top_builddir)/xrl/targets/libospfv3base.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6xif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_ospfv2_SOURCES		 = xorp_ospfv2.cc

xorp_ospfv3_SOURCES		 = xorp_ospfv3.cc

test_packet_SOURCES		 = test_common.hh
test_packet_SOURCES		+= test_packet.cc

test_checksum_SOURCES		 = test_checksum.cc

test_peering_SOURCES		 = test_peering.cc

test_routing_SOURCES		 = test_common.hh
test_routing_SOURCES		+= test_routing.cc

test_routing_database_SOURCES	 = test_common.hh
test_routing_database_SOURCES	+= test_routing_database.cc

test_routing_table_SOURCES	 = test_routing_table.cc

test_build_lsa_main_SOURCES	 = test_args.hh
test_build_lsa_main_SOURCES	+= test_build_lsa.hh
test_build_lsa_main_SOURCES	+= test_build_lsa.cc
test_build_lsa_main_SOURCES	+= test_build_lsa_main.cc

test_routing_interactive_SOURCES	 = test_args.hh
test_routing_interactive_SOURCES	+= test_build_lsa.hh
test_routing_interactive_SOURCES	+= test_build_lsa.cc
test_routing_interactive_SOURCES	+= test_routing_interactive.cc

# -- Library Sources
libospf_la_SOURCES  = auth.hh
libospf_la_SOURCES  += area_router.hh
libospf_la_SOURCES  += exceptions.hh
libospf_la_SOURCES  += debug_io.hh
libospf_la_SOURCES  += delay_queue.hh
libospf_la_SOURCES  += external.hh
libospf_la_SOURCES  += fletcher_checksum.hh
libospf_la_SOURCES  += lsa.hh
libospf_la_SOURCES  += ospf.hh
libospf_la_SOURCES  += packet.hh
libospf_la_SOURCES  += peer_manager.hh
libospf_la_SOURCES  += peer.hh
libospf_la_SOURCES  += policy_varrw.hh
libospf_la_SOURCES  += routing_table.hh
libospf_la_SOURCES  += xrl_io.hh
libospf_la_SOURCES  += xrl_target.hh
libospf_la_SOURCES  += vertex.hh
libospf_la_SOURCES  += vlink.hh

libospf_la_SOURCES  += auth.cc
libospf_la_SOURCES  += area_router.cc
libospf_la_SOURCES  += external.cc
libospf_la_SOURCES  += fletcher_checksum.cc
libospf_la_SOURCES  += lsa.cc
libospf_la_SOURCES  += ospf.cc
libospf_la_SOURCES  += packet.cc
libospf_la_SOURCES  += peer_manager.cc
libospf_la_SOURCES  += peer.cc
libospf_la_SOURCES  += policy_varrw.cc
libospf_la_SOURCES  += routing_table.cc
libospf_la_SOURCES  += xrl_io.cc
libospf_la_SOURCES  += xrl_target.cc
libospf_la_SOURCES  += vlink.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 =

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
