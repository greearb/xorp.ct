## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libproto/Makefile.am,v 1.7 2004/11/02 23:15:22 atanu Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_spt

# -- No-install Programs
noinst_PROGRAMS  = test_spt

# -- No-install Libraries
noinst_LTLIBRARIES = libproto.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_spt_SOURCES	 = test_spt.cc
test_spt_SOURCES	+= spt.hh
test_spt_SOURCES	+= spt.cc

# -- Library Sources
libproto_la_SOURCES	 = libproto_module.h

libproto_la_SOURCES	+= proto_node.hh
libproto_la_SOURCES	+= proto_node_cli.hh
libproto_la_SOURCES	+= proto_register.hh
libproto_la_SOURCES	+= proto_state.hh
libproto_la_SOURCES	+= proto_unit.hh
libproto_la_SOURCES	+= spt.hh

libproto_la_SOURCES	+= proto_node_cli.cc
libproto_la_SOURCES	+= proto_register.cc
libproto_la_SOURCES	+= proto_state.cc
libproto_la_SOURCES	+= proto_unit.cc
libproto_la_SOURCES	+= spt.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

