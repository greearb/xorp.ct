## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libproto/Makefile.am,v 1.6 2003/08/01 02:11:48 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_spt

# -- No-install Programs
noinst_PROGRAMS  = test_spt

# -- No-install Libraries
noinst_LIBRARIES = libproto.a

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)
LDADD		 += $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_spt_SOURCES	= test_spt.cc
test_spt_SOURCES	+= spt.hh
test_spt_SOURCES	+= spt.cc

# -- Library Sources
libproto_a_SOURCES  = libproto_module.h

libproto_a_SOURCES += proto_node.hh
libproto_a_SOURCES += proto_node_cli.hh
libproto_a_SOURCES += proto_register.hh
libproto_a_SOURCES += proto_state.hh
libproto_a_SOURCES += proto_unit.hh
libproto_a_SOURCES += spt.hh

libproto_a_SOURCES += proto_node_cli.cc
libproto_a_SOURCES += proto_register.cc
libproto_a_SOURCES += proto_state.cc
libproto_a_SOURCES += proto_unit.cc
libproto_a_SOURCES += spt.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

