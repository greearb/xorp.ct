## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libproto/Makefile.am,v 1.19 2007/05/19 01:52:43 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- No-install Programs
check_PROGRAMS	 = test_checksum
check_PROGRAMS	+= test_config_node_id
check_PROGRAMS	+= test_packet
check_PROGRAMS	+= test_spt

# -- Test Programs
TESTS		 = test_checksum$(EXEEXT)
TESTS		+= test_config_node_id$(EXEEXT)
TESTS		+= test_packet$(EXEEXT)
TESTS		+= test_spt$(EXEEXT)

# -- Libraries
lib_LTLIBRARIES = libproto.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(lib_LTLIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_checksum_SOURCES	= test_checksum.cc
test_config_node_id_SOURCES = test_config_node_id.cc
test_packet_SOURCES	= test_packet.cc
test_spt_SOURCES	= test_spt.cc

# -- Library Sources
libproto_la_SOURCES	 = libproto_module.h
libproto_la_SOURCES	+= checksum.h

libproto_la_SOURCES	+= config_node_id.hh
libproto_la_SOURCES	+= packet.hh
libproto_la_SOURCES	+= proto_node.hh
libproto_la_SOURCES	+= proto_node_cli.hh
libproto_la_SOURCES	+= proto_state.hh
libproto_la_SOURCES	+= proto_unit.hh
libproto_la_SOURCES	+= spt.hh

libproto_la_SOURCES	+= checksum.c

libproto_la_SOURCES	+= packet.cc
libproto_la_SOURCES	+= proto_node_cli.cc
libproto_la_SOURCES	+= proto_state.cc
libproto_la_SOURCES	+= proto_unit.cc
libproto_la_SOURCES	+= spt.cc

libproto_la_LIBADD       = $(top_builddir)/libxorp/libxorp.la

# -- Extra files and directories to add to the distribution
EXTRA_DIST		 = spt_graph1

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
