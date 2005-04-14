## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/test/Makefile.am,v 1.2 2004/09/17 19:42:03 pavlin Exp $
##

# -- Test Programs
TESTS            = dummy_test.sh


# -- No-install Programs
noinst_PROGRAMS  = compilepolicy
noinst_PROGRAMS += execpolicy

# compilepolicy
compilepolicy_SOURCES		 = compilepolicy.cc
compilepolicy_SOURCES		+= filter_manager_fake.hh
compilepolicy_SOURCES		+= process_watch_fake.hh

compilepolicy_SOURCES		+= filter_manager_fake.cc
compilepolicy_SOURCES		+= process_watch_fake.cc

compilepolicy_SOURCES		+= lex.yy_compile_policy.cc
compilepolicy_SOURCES		+= yacc.yy_compile_policy.cc
compilepolicy_SOURCES		+= yacc.yy_compile_policy.cc.h

compilepolicy_LDADD		 = ../libpolicy.la
compilepolicy_LDADD		+= ../common/libpolicycommon.la
compilepolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.la
compilepolicy_LDADD		+= $(top_builddir)/libcomm/libcomm.la
compilepolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.la



# execpolicy
execpolicy_SOURCES		 = execpolicy.cc
execpolicy_SOURCES		+= file_varrw.hh

execpolicy_SOURCES		+= file_varrw.cc

execpolicy_LDADD		 = ../backend/libpolicybackend.la
execpolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.la
execpolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.la


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
