## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mld6igmp/Makefile.am,v 1.33 2007/04/18 01:18:10 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_igmp xorp_mld

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS	 = 

# -- No-install Libraries
noinst_LTLIBRARIES = libmld6igmp.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.la
### XORP misc libraries
LDADD		+= $(top_builddir)/mrt/libmrt.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_igmp_SOURCES	 = xorp_igmp.cc
xorp_mld_SOURCES	 = xorp_mld.cc

# -- Library Sources
libmld6igmp_la_SOURCES  = mld6igmp_module.h
libmld6igmp_la_SOURCES += igmp_proto.h
libmld6igmp_la_SOURCES += mld6_proto.h
libmld6igmp_la_SOURCES += mld6igmp_group_record.hh
libmld6igmp_la_SOURCES += mld6igmp_node.hh
libmld6igmp_la_SOURCES += mld6igmp_node_cli.hh
libmld6igmp_la_SOURCES += mld6igmp_source_record.hh
libmld6igmp_la_SOURCES += mld6igmp_vif.hh
libmld6igmp_la_SOURCES += xrl_mld6igmp_node.hh

libmld6igmp_la_SOURCES += mld6igmp_config.cc
libmld6igmp_la_SOURCES += mld6igmp_group_record.cc
libmld6igmp_la_SOURCES += mld6igmp_node.cc
libmld6igmp_la_SOURCES += mld6igmp_node_cli.cc
libmld6igmp_la_SOURCES += mld6igmp_proto.cc
libmld6igmp_la_SOURCES += mld6igmp_source_record.cc
libmld6igmp_la_SOURCES += mld6igmp_vif.cc
libmld6igmp_la_SOURCES += xrl_mld6igmp_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = command_mld6igmp
EXTRA_DIST	 += docs
EXTRA_DIST	 += xrl_mld6igmp_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
