## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libfeaclient/Makefile.am,v 1.7 2004/11/12 02:44:42 bms Exp $
##

# -- Real Programs
bin_PROGRAMS		 =

# -- No-install Libraries
noinst_LTLIBRARIES	 = libfeaclient.la

# -- Library Sources
libfeaclient_la_SOURCES	 = libfeaclient_module.h

libfeaclient_la_SOURCES	+= ifmgr_atoms.hh
libfeaclient_la_SOURCES	+= ifmgr_cmd_base.hh
libfeaclient_la_SOURCES	+= ifmgr_cmds.hh
libfeaclient_la_SOURCES	+= ifmgr_cmd_queue.hh
libfeaclient_la_SOURCES	+= ifmgr_xrl_replicator.hh
libfeaclient_la_SOURCES	+= ifmgr_xrl_mirror.hh

libfeaclient_la_SOURCES	+= ifmgr_atoms.cc
libfeaclient_la_SOURCES	+= ifmgr_cmds.cc
libfeaclient_la_SOURCES	+= ifmgr_cmd_queue.cc
libfeaclient_la_SOURCES	+= ifmgr_xrl_replicator.cc
libfeaclient_la_SOURCES	+= ifmgr_xrl_mirror.cc

# -- Test Programs
TESTS			 = test_local_copy
TESTS			+= test_remote_copy

# -- No-install Programs
check_PROGRAMS		 = test_local_copy
check_PROGRAMS		+= test_remote_copy

# -- Libraries and library flags to link the binary programs
LDADD			 = ./libfeaclient.la
LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD			+= $(top_builddir)/libxipc/libxipc.la
LDADD			+= $(top_builddir)/libcomm/libcomm.la
LDADD			+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_local_copy_SOURCES	 = test_local_copy.cc

test_remote_copy_SOURCES	 = test_remote_copy.cc
test_remote_copy_LDADD	= $(LDADD)
test_remote_copy_LDADD	+= $(top_builddir)/xrl/targets/libtestfeaifmgrmirrorbase.la
test_remote_copy_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
test_remote_copy_LDADD  += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
test_remote_copy_LDADD	+= $(top_builddir)/libxipc/libfinder.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
