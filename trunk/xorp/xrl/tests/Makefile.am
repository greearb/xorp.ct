## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/tests/Makefile.am,v 1.3 2003/03/04 23:41:26 hodson Exp $
##

TESTS = test_generated
noinst_PROGRAMS = $(TESTS)

test_generated_SOURCES  = test_generated.cc
test_generated_SOURCES += test_tgt.hh test_tgt.cc
test_generated_SOURCES += test_xifs.hh test_xifs.cc

test_generated_LDADD    = $(top_builddir)/xrl/interfaces/libcommonxif.a
test_generated_LDADD   += $(top_builddir)/xrl/interfaces/libtestxif.a
test_generated_LDADD   += $(top_builddir)/xrl/targets/libtestbase.a
test_generated_LDADD   += $(top_builddir)/libxipc/libfinder.a
test_generated_LDADD   += $(top_builddir)/libxipc/libxipc.a
test_generated_LDADD   += $(top_builddir)/libcomm/libcomm.a
test_generated_LDADD   += $(top_builddir)/libxorp/libxorp.a


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

