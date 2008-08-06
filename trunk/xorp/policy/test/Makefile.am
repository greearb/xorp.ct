## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/test/Makefile.am,v 1.6 2008/08/06 08:04:51 abittau Exp $
##

# -- Test Programs
TESTS            = dummy_test.sh

# -- No-install Programs
noinst_PROGRAMS  = compilepolicy
noinst_PROGRAMS += execpolicy
noinst_PROGRAMS += policybench

# compilepolicy
compilepolicy_SOURCES		 = compilepolicy.cc
compilepolicy_SOURCES		+= filter_manager_fake.hh
compilepolicy_SOURCES		+= process_watch_fake.hh
compilepolicy_SOURCES		+= compilepolicy.hh
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
execpolicy_LDADD		+= ../common/libpolicycommon.la
execpolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.la
execpolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# policybench
policybench_SOURCES		 = policybench.cc
policybench_SOURCES		+= file_varrw.cc

policybench_LDADD		 = $(top_builddir)/bgp/libbgp.la
policybench_LDADD               += $(top_builddir)/xrl/targets/libbgpbase.la
policybench_LDADD        	+= $(top_builddir)/policy/backend/libpolicybackend.la
policybench_LDADD               += $(top_builddir)/libfeaclient/libfeaclient.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libribxif.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libbgpmibtrapsxif.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libprofileclientxif.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
policybench_LDADD               += $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
policybench_LDADD               += $(top_builddir)/libxipc/libxipc.la
policybench_LDADD               += $(top_builddir)/libcomm/libcomm.la
policybench_LDADD               += $(top_builddir)/libxorp/libxorp.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
