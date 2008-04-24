## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# -- Subdirectories
SUBDIRS = . tools

# Redefine the bin installation directory
bindir = $(prefix)/olsr

# Point preprocessor at relocated olsr module
AM_CPPFLAGS	 = -I$(top_srcdir)/contrib -I$(top_srcdir)/contrib/olsr

# -- Real Programs
bin_PROGRAMS	 = xorp_olsr4

# -- Test Programs
check_PROGRAMS	 = test_olsr_types
check_PROGRAMS	+= test_message
check_PROGRAMS	+= test_faces
check_PROGRAMS	+= test_simulator

TESTS		 = test_olsr_types$(EXEEXT)
TESTS		+= test_message$(EXEEXT)
TESTS		+= test_faces$(EXEEXT)
TESTS		+= test_routing1.py

# -- No-install Libtool Libraries
noinst_LTLIBRARIES	 = libolsr.la
noinst_LTLIBRARIES	+= libolsrtest.la
noinst_LTLIBRARIES	+= libolsrxrl.la

# -- Libraries and library flags to link the binary programs
LDADD		 = libolsr.la
LDADD		+= $(top_builddir)/xrl/targets/libolsr4base.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/policy/common/libpolicycommon.la
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

TEST_LDADD	 = libolsrtest.la
TEST_LDADD	+= $(LDADD)

# -- Program Sources
xorp_olsr4_SOURCES		 = xorp_olsr.cc
xorp_olsr4_LDADD		 = libolsrxrl.la $(LDADD)

test_olsr_types_SOURCES		 = test_olsr_types.cc
test_olsr_types_LDADD		 = $(TEST_LDADD)

test_message_SOURCES		 = test_common.hh
test_message_SOURCES		+= test_message.cc
test_message_LDADD		 = $(TEST_LDADD)

test_faces_SOURCES		 = test_faces.cc
test_faces_LDADD		 = $(TEST_LDADD)

test_simulator_SOURCES		 = test_simulator.cc
test_simulator_LDADD		 = $(TEST_LDADD)

# -- Library Sources
libolsr_la_SOURCES   = exceptions.hh
libolsr_la_SOURCES  += external.hh
libolsr_la_SOURCES  += face.hh
libolsr_la_SOURCES  += face_manager.hh
libolsr_la_SOURCES  += io.hh
libolsr_la_SOURCES  += link.hh
libolsr_la_SOURCES  += message.hh
libolsr_la_SOURCES  += neighbor.hh
libolsr_la_SOURCES  += neighborhood.hh
libolsr_la_SOURCES  += olsr.hh
libolsr_la_SOURCES  += olsr_types.hh
libolsr_la_SOURCES  += policy_varrw.hh
libolsr_la_SOURCES  += profile_vars.hh
libolsr_la_SOURCES  += route_manager.hh
libolsr_la_SOURCES  += topology.hh
libolsr_la_SOURCES  += trace.hh
libolsr_la_SOURCES  += twohop.hh
libolsr_la_SOURCES  += vertex.hh

libolsr_la_SOURCES  += external.cc
libolsr_la_SOURCES  += face.cc
libolsr_la_SOURCES  += face_manager.cc
libolsr_la_SOURCES  += link.cc
libolsr_la_SOURCES  += message.cc
libolsr_la_SOURCES  += neighbor.cc
libolsr_la_SOURCES  += neighborhood.cc
libolsr_la_SOURCES  += olsr.cc
libolsr_la_SOURCES  += olsr_types.cc
libolsr_la_SOURCES  += policy_varrw.cc
libolsr_la_SOURCES  += route_manager.cc
libolsr_la_SOURCES  += topology.cc
libolsr_la_SOURCES  += twohop.cc

libolsrxrl_la_SOURCES  = xrl_io.hh
libolsrxrl_la_SOURCES += xrl_queue.hh
libolsrxrl_la_SOURCES += xrl_port.hh
libolsrxrl_la_SOURCES += xrl_target.hh

libolsrxrl_la_SOURCES += xrl_io.cc
libolsrxrl_la_SOURCES += xrl_queue.cc
libolsrxrl_la_SOURCES += xrl_port.cc
libolsrxrl_la_SOURCES += xrl_target.cc

libolsrtest_la_SOURCES  = debug_io.hh
libolsrtest_la_SOURCES += emulate_net.hh
libolsrtest_la_SOURCES += test_args.hh
libolsrtest_la_SOURCES += test_common.hh

libolsrtest_la_SOURCES += debug_io.cc
libolsrtest_la_SOURCES += emulate_net.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 =

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
