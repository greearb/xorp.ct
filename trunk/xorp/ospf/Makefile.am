## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/ospf/Makefile.am,v 1.13 2005/09/05 18:58:04 atanu Exp $
##

# -- Subdirectories
SUBDIRS =

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_ospfv2 xorp_ospfv3

# -- Test Programs
check_PROGRAMS	 = test_packet
check_PROGRAMS	+= test_checksum
check_PROGRAMS	+= test_peering
check_PROGRAMS	+= test_routing

TESTS		 = test_packet$(EXEEXT)
TESTS		+= test_checksum$(EXEEXT)
TESTS		+= test_peering$(EXEEXT)
TESTS		+= test_routing$(EXEEXT)

# -- No-install Libtool Libraries
noinst_LTLIBRARIES	 = libospf.la

# -- Libraries and library flags to link the binary programs
LDADD		 = libospf.la
LDADD		+= $(top_builddir)/xrl/targets/libospfv2base.la
LDADD		+= $(top_builddir)/xrl/targets/libospfv3base.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la
LDADD		+= $(top_builddir)/libproto/libproto.la

# -- Program Sources
xorp_ospfv2_SOURCES		= xorp_ospfv2.cc

xorp_ospfv3_SOURCES		= xorp_ospfv3.cc

test_packet_SOURCES		= test_packet.cc

test_checksum_SOURCES		= test_checksum.cc

test_peering_SOURCES		= test_peering.cc

test_routing_SOURCES		= test_routing.cc

# -- Library Sources
libospf_la_SOURCES  = area_router.hh
libospf_la_SOURCES  += exceptions.hh
libospf_la_SOURCES  += debug_io.hh
libospf_la_SOURCES  += delay_queue.hh
libospf_la_SOURCES  += fletcher_checksum.hh
libospf_la_SOURCES  += lsa.hh
libospf_la_SOURCES  += ospf.hh
libospf_la_SOURCES  += packet.hh
libospf_la_SOURCES  += peer_manager.hh
libospf_la_SOURCES  += peer.hh
libospf_la_SOURCES  += routing_table.hh
libospf_la_SOURCES  += xrl_io.hh
libospf_la_SOURCES  += xrl_target.hh
libospf_la_SOURCES  += vertex.hh

libospf_la_SOURCES  += area_router.cc
libospf_la_SOURCES  += fletcher_checksum.cc
libospf_la_SOURCES  += lsa.cc
libospf_la_SOURCES  += ospf.cc
libospf_la_SOURCES  += packet.cc
libospf_la_SOURCES  += peer_manager.cc
libospf_la_SOURCES  += peer.cc
libospf_la_SOURCES  += routing_table.cc
libospf_la_SOURCES  += xrl_io.cc
libospf_la_SOURCES  += xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 =

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
