## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mrt/Makefile.am,v 1.16 2002/12/09 12:47:46 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
bin_PROGRAMS	+= test_mrt test_mtimer test_mrib

noinst_LIBRARIES = libmrt.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_mrt_SOURCES	= test_mrt.cc
test_mtimer_SOURCES	= test_mtimer.cc
test_mrib_SOURCES	= test_mrib.cc

# -- Library Sources
libmrt_a_SOURCES  = mrt_module.h
libmrt_a_SOURCES += buffer.h
libmrt_a_SOURCES += counter.h
libmrt_a_SOURCES += inet_cksum.h
libmrt_a_SOURCES += max_vifs.h
libmrt_a_SOURCES += multicast_defs.h
libmrt_a_SOURCES += netstream_access.h
libmrt_a_SOURCES += random.h
libmrt_a_SOURCES += timer.h

libmrt_a_SOURCES += mifset.hh
libmrt_a_SOURCES += mrt.hh
libmrt_a_SOURCES += mrib_table.hh
libmrt_a_SOURCES += timer.hh

libmrt_a_SOURCES += buffer.c
libmrt_a_SOURCES += inet_cksum.c
libmrt_a_SOURCES += random.c
libmrt_a_SOURCES += timer.c

libmrt_a_SOURCES += mifset.cc
libmrt_a_SOURCES += mrib_table.cc
libmrt_a_SOURCES += timer2xorp.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = include


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

