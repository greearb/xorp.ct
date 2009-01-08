## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/tests/Makefile.am,v 1.8 2004/11/23 22:43:49 bms Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- No-install Programs
check_PROGRAMS	 = test_generated

# -- Test Programs
TESTS		 = test_generated$(EXEEXT)

# -- No-install Libraries
noinst_LTLIBRARIES =

test_generated_SOURCES  = test_generated.cc
test_generated_SOURCES += test_tgt.hh test_tgt.cc
test_generated_SOURCES += test_xifs.hh test_xifs.cc

test_generated_LDADD    = $(top_builddir)/xrl/interfaces/libcommonxif.la
test_generated_LDADD   += $(top_builddir)/xrl/interfaces/libtestxif.la
test_generated_LDADD   += $(top_builddir)/xrl/targets/libtestbase.la
test_generated_LDADD   += $(top_builddir)/libxipc/libfinder.la
test_generated_LDADD   += $(top_builddir)/libxipc/libxipc.la
test_generated_LDADD   += $(top_builddir)/libcomm/libcomm.la
test_generated_LDADD   += $(top_builddir)/libxorp/libxorp.la


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

