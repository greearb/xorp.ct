## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mrt/Makefile.am,v 1.7 2004/11/12 02:44:45 bms Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = 

# -- No-install Programs
check_PROGRAMS  = test_mrt
check_PROGRAMS	+= test_mrib

# -- No-install Libraries
noinst_LTLIBRARIES = libmrt.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_mrt_SOURCES	= test_mrt.cc
test_mrib_SOURCES	= test_mrib.cc

# -- Library Sources
libmrt_la_SOURCES  = mrt_module.h
libmrt_la_SOURCES += buffer.h
libmrt_la_SOURCES += inet_cksum.h
libmrt_la_SOURCES += max_vifs.h
libmrt_la_SOURCES += multicast_defs.h
libmrt_la_SOURCES += netstream_access.h
libmrt_la_SOURCES += random.h

libmrt_la_SOURCES += mifset.hh
libmrt_la_SOURCES += mrt.hh
libmrt_la_SOURCES += mrib_table.hh

libmrt_la_SOURCES += buffer.c
libmrt_la_SOURCES += inet_cksum.c
libmrt_la_SOURCES += random.c

libmrt_la_SOURCES += mifset.cc
libmrt_la_SOURCES += mrib_table.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = include


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
