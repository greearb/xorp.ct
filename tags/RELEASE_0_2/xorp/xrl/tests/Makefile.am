## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/tests/Makefile.am,v 1.2 2003/02/27 19:54:15 hodson Exp $
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
