## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libproto/Makefile.am,v 1.4 2003/05/07 21:58:08 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
check_PROGRAMS	 = 
TESTS		 = $(check_PROGRAMS)

# -- No-install Programs
noinst_PROGRAMS  = 

# -- No-install Libraries
noinst_LIBRARIES = libproto.a

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
#test_proto_SOURCES	= test_proto.cc

# -- Library Sources
libproto_a_SOURCES  = libproto_module.h

libproto_a_SOURCES += proto_node.hh
libproto_a_SOURCES += proto_node_cli.hh
libproto_a_SOURCES += proto_register.hh
libproto_a_SOURCES += proto_state.hh
libproto_a_SOURCES += proto_unit.hh

libproto_a_SOURCES += proto_node_cli.cc
libproto_a_SOURCES += proto_register.cc
libproto_a_SOURCES += proto_state.cc
libproto_a_SOURCES += proto_unit.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

