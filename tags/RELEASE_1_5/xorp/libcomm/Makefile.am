## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libcomm/Makefile.am,v 1.10 2005/12/22 11:43:32 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 =

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS  = test_comm
check_PROGRAMS += test_connect

# -- No-install Libraries
noinst_LTLIBRARIES = libcomm.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_comm_SOURCES = test_comm.c
test_connect_SOURCES = test_connect.cc

# -- Miscellaneous Setup
# XXX: Linking C programs with C++ libraries requires C++ linker
test_comm_LINK = $(CXXLINK)

# -- Library Sources
libcomm_la_SOURCES  = comm_module.h
libcomm_la_SOURCES += comm_api.h
libcomm_la_SOURCES += comm_private.h

libcomm_la_SOURCES += comm_sock.c
libcomm_la_SOURCES += comm_user.c


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

