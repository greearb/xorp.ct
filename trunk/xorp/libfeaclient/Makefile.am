## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libfeaclient/Makefile.am,v 1.3 2003/08/28 00:08:54 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS		=

# -- No-install Libraries
noinst_LIBRARIES	= libfeaclient.a

# -- Library Sources
libfeaclient_a_SOURCES  = libfeaclient_module.h

libfeaclient_a_SOURCES += ifmgr_atoms.hh
libfeaclient_a_SOURCES += ifmgr_cmd_base.hh
libfeaclient_a_SOURCES += ifmgr_cmds.hh
libfeaclient_a_SOURCES += ifmgr_cmd_queue.hh
libfeaclient_a_SOURCES += ifmgr_xrl_replicator.hh
libfeaclient_a_SOURCES += xrl_ifmgr_mirror.hh

libfeaclient_a_SOURCES += ifmgr_atoms.cc
libfeaclient_a_SOURCES += ifmgr_cmds.cc
libfeaclient_a_SOURCES += ifmgr_cmd_queue.cc
libfeaclient_a_SOURCES += ifmgr_xrl_replicator.cc
libfeaclient_a_SOURCES += xrl_ifmgr_mirror.cc

# -- Test Programs
TESTS			= test_local_copy
#__TESTS		       += test_remote_copy

# -- No-install Programs
noinst_PROGRAMS		= test_local_copy
#__noinst_PROGRAMS	       += test_remote_copy

# -- Libraries and library flags to link the binary programs
LDADD			= ./libfeaclient.a
LDADD		       += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
LDADD		       += $(top_builddir)/libxipc/libxipc.a
LDADD		       += $(top_builddir)/libcomm/libcomm.a
LDADD		       += $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_local_copy_SOURCES = test_local_copy.cc

# XXX Not yet
#__test_remote_copy_SOURCES = test_remote_copy.cc
#__test_remote_copy_LDADD	= $(top_builddir)/libxipc/libfinder.a $(LDADD)

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
