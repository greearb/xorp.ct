## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libfeaclient/Makefile.am,v 1.11 2005/12/21 09:42:55 bms Exp $
##

# -- Real Programs
bin_PROGRAMS		 =

# -- Libraries
lib_LTLIBRARIES	 = libfeaclient.la

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

libfeaclient_la_LIBADD	= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
libfeaclient_la_LIBADD += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
libfeaclient_la_LIBADD += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
libfeaclient_la_LIBADD += $(top_builddir)/libxipc/libxipc.la
libfeaclient_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaclient_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# -- No-install Programs
check_PROGRAMS		 = test_local_copy
check_PROGRAMS		+= test_remote_copy

# -- Test Programs
TESTS			 = test_local_copy$(EXEEXT)
TESTS			+= test_remote_copy$(EXEEXT)


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
test_remote_copy_LDADD	+= $(top_builddir)/libxorp/libxorp.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
