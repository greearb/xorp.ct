## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mld6igmp/Makefile.am,v 1.20 2004/05/04 00:11:29 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_igmp xorp_mld

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  = test_mld6igmp

# -- No-install Libraries
noinst_LIBRARIES = libmld6igmp.a

# -- Libraries and library flags to link the binary programs
### MLD6IGMP libraries
LDADD		 = $(noinst_LIBRARIES)
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.a
### MFEA libraries
LDADD		+= $(top_builddir)/fea/libfea.a
# FEA-specific begin
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
# FEA-specific end
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.a
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= @CLI_CURSES_LIB@
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.a
LDADD		+= $(top_builddir)/libxipc/libfinder.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
xorp_igmp_SOURCES	 = xorp_igmp.cc
xorp_mld_SOURCES	 = xorp_mld.cc

test_mld6igmp_SOURCES	 = test_mld6igmp.cc
test_mld6igmp_LDADD	 = $(LDADD)
test_mld6igmp_LDADD	+= $(top_builddir)/libfeaclient/libfeaclient.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.a

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
libmld6igmp_a_SOURCES += mld6igmp_config.cc
libmld6igmp_a_SOURCES += mld6igmp_member_query.cc
libmld6igmp_a_SOURCES += mld6igmp_node.cc
libmld6igmp_a_SOURCES += mld6igmp_node_cli.cc
libmld6igmp_a_SOURCES += mld6igmp_proto.cc
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

INCLUDES =	-I$(top_srcdir)

