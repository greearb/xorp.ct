## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mld6igmp/Makefile.am,v 1.25 2004/11/23 22:43:45 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_igmp xorp_mld

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS  = test_mld6igmp

# -- No-install Libraries
noinst_LTLIBRARIES = libmld6igmp.la

# -- Libraries and library flags to link the binary programs
### MLD6IGMP libraries
LDADD		 = $(noinst_LTLIBRARIES)
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.la
### MFEA libraries
LDADD		+= $(top_builddir)/fea/libfea.la
# FEA-specific begin
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4client.la
LDADD		+= $(top_builddir)/xrl/targets/libfeabase.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
# FEA-specific end
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.la
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.la
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.la
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= @CLI_CURSES_LIB@
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libfinder.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_igmp_SOURCES	 = xorp_igmp.cc
xorp_mld_SOURCES	 = xorp_mld.cc

test_mld6igmp_SOURCES	 = test_mld6igmp.cc
test_mld6igmp_LDADD	 = $(LDADD)
test_mld6igmp_LDADD	+= $(top_builddir)/libfeaclient/libfeaclient.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
test_mld6igmp_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.la
test_mld6igmp_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Library Sources
libmld6igmp_la_SOURCES  = mld6igmp_module.h
libmld6igmp_la_SOURCES += igmp_proto.h
libmld6igmp_la_SOURCES += mld6_proto.h
libmld6igmp_la_SOURCES += mld6igmp_member_query.hh
libmld6igmp_la_SOURCES += mld6igmp_node.hh
libmld6igmp_la_SOURCES += mld6igmp_node_cli.hh
libmld6igmp_la_SOURCES += mld6igmp_private.hh
libmld6igmp_la_SOURCES += mld6igmp_vif.hh
libmld6igmp_la_SOURCES += xrl_mld6igmp_node.hh

libmld6igmp_la_SOURCES += igmp_proto.cc
libmld6igmp_la_SOURCES += mld6_proto.cc
libmld6igmp_la_SOURCES += mld6igmp_config.cc
libmld6igmp_la_SOURCES += mld6igmp_member_query.cc
libmld6igmp_la_SOURCES += mld6igmp_node.cc
libmld6igmp_la_SOURCES += mld6igmp_node_cli.cc
libmld6igmp_la_SOURCES += mld6igmp_proto.cc
libmld6igmp_la_SOURCES += mld6igmp_vif.cc
libmld6igmp_la_SOURCES += xrl_mld6igmp_node.cc

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

