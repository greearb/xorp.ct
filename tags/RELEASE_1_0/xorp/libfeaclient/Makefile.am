## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libfeaclient/Makefile.am,v 1.5 2003/09/10 19:21:33 hodson Exp $
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
libfeaclient_a_SOURCES += ifmgr_xrl_mirror.hh

libfeaclient_a_SOURCES += ifmgr_atoms.cc
libfeaclient_a_SOURCES += ifmgr_cmds.cc
libfeaclient_a_SOURCES += ifmgr_cmd_queue.cc
libfeaclient_a_SOURCES += ifmgr_xrl_replicator.cc
libfeaclient_a_SOURCES += ifmgr_xrl_mirror.cc

# -- Test Programs
TESTS			= test_local_copy
TESTS		       += test_remote_copy

# -- No-install Programs
noinst_PROGRAMS		= test_local_copy
noinst_PROGRAMS	       += test_remote_copy

# -- Libraries and library flags to link the binary programs
LDADD			= ./libfeaclient.a
LDADD		       += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
LDADD		       += $(top_builddir)/libxipc/libxipc.a
LDADD		       += $(top_builddir)/libcomm/libcomm.a
LDADD		       += $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_local_copy_SOURCES = test_local_copy.cc

test_remote_copy_SOURCES = test_remote_copy.cc
test_remote_copy_LDADD	= $(LDADD)
test_remote_copy_LDADD	+= $(top_builddir)/xrl/targets/libtestfeaifmgrmirrorbase.a
test_remote_copy_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
test_remote_copy_LDADD  += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
test_remote_copy_LDADD	+= $(top_builddir)/libxipc/libfinder.a

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
