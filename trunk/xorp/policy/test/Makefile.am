##
## $XORP$
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

compilepolicy_LDADD		 = ../libpolicy.a
compilepolicy_LDADD		+= ../common/libpolicycommon.a
compilepolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.a
compilepolicy_LDADD		+= $(top_builddir)/libcomm/libcomm.a
compilepolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.a



# execpolicy
execpolicy_SOURCES		 = execpolicy.cc
execpolicy_SOURCES		+= file_varrw.hh

execpolicy_SOURCES		+= file_varrw.cc

execpolicy_LDADD		 = ../backend/libpolicybackend.a
execpolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.a
execpolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.a



INCLUDES =      -I$(top_srcdir)
