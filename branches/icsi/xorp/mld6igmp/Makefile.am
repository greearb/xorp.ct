## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mld6igmp/Makefile.am,v 1.23 2002/12/09 12:23:15 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
bin_PROGRAMS	+= test_mld6igmp

noinst_LIBRARIES = libmld6igmp.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
### MLD6IGMP libraries
LDADD		 = $(noinst_LIBRARIES)
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.a
### MFEA libraries
LDADD		+= $(top_builddir)/mfea/libmfea.a
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.a
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= -lcurses
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_mld6igmp_SOURCES = test_mld6igmp.cc

# -- Library Sources
libmld6igmp_a_SOURCES  = mld6igmp_module.h
libmld6igmp_a_SOURCES += igmp_proto.h
libmld6igmp_a_SOURCES += mld6_proto.h
libmld6igmp_a_SOURCES += mld6igmp_member_query.hh
libmld6igmp_a_SOURCES += mld6igmp_node.hh
libmld6igmp_a_SOURCES += mld6igmp_node_cli.hh
libmld6igmp_a_SOURCES += mld6igmp_private.hh
libmld6igmp_a_SOURCES += mld6igmp_vif.hh
libmld6igmp_a_SOURCES += xrl_mld6igmp_node.hh

libmld6igmp_a_SOURCES += igmp_proto.cc
libmld6igmp_a_SOURCES += mld6_proto.cc
libmld6igmp_a_SOURCES += mld6igmp_member_query.cc
libmld6igmp_a_SOURCES += mld6igmp_node.cc
libmld6igmp_a_SOURCES += mld6igmp_node_cli.cc
libmld6igmp_a_SOURCES += mld6igmp_vif.cc
libmld6igmp_a_SOURCES += xrl_mld6igmp_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = configure_mld6igmp
EXTRA_DIST	 += docs
EXTRA_DIST	 += xrl_mld6igmp_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

