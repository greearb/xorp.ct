## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/test/Makefile.am,v 1.10 2009/01/29 18:57:03 jtc Exp $
##

# -- Test Programs
TESTS            = dummy_test.sh

# -- No-install Programs
noinst_PROGRAMS  = 

# compilepolicy
noinst_PROGRAMS += compilepolicy
compilepolicy_SOURCES		 = compilepolicy.cc
compilepolicy_SOURCES		+= filter_manager_fake.hh
compilepolicy_SOURCES		+= process_watch_fake.hh
compilepolicy_SOURCES		+= compilepolicy.hh
compilepolicy_SOURCES		+= filter_manager_fake.cc
compilepolicy_SOURCES		+= process_watch_fake.cc
compilepolicy_SOURCES		+= lex.yy_compile_policy.cc
compilepolicy_SOURCES		+= yacc.yy_compile_policy.cc
compilepolicy_SOURCES		+= yacc.yy_compile_policy.cc.h

compilepolicy_SOURCES		+= $(srcdir)/../policy_target.cc
compilepolicy_SOURCES		+= $(srcdir)/../policy_target.hh

compilepolicy_LDADD		 = ../libpolicy.la
compilepolicy_LDADD		+= ../common/libpolicycommon.la
compilepolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.la
compilepolicy_LDADD		+= $(top_builddir)/libcomm/libcomm.la
compilepolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# execpolicy
noinst_PROGRAMS += execpolicy
execpolicy_SOURCES		 = execpolicy.cc
execpolicy_SOURCES		+= file_varrw.hh
execpolicy_SOURCES		+= file_varrw.cc

execpolicy_LDADD		 = ../backend/libpolicybackend.la
execpolicy_LDADD		+= ../common/libpolicycommon.la
execpolicy_LDADD		+= $(top_builddir)/libxipc/libxipc.la
execpolicy_LDADD		+= $(top_builddir)/libcomm/libcomm.la
execpolicy_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# policybench
# noinst_PROGRAMS += policybench
# policybench_SOURCES		 = policybench.cc
# policybench_SOURCES		+= file_varrw.cc

# policybench_LDADD		 = $(top_builddir)/bgp/libbgp.la
# policybench_LDADD               += $(top_builddir)/xrl/targets/libbgpbase.la
# policybench_LDADD        	+= $(top_builddir)/policy/backend/libpolicybackend.la
# policybench_LDADD               += $(top_builddir)/libfeaclient/libfeaclient.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libribxif.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libbgpmibtrapsxif.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libprofileclientxif.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
# policybench_LDADD               += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
# policybench_LDADD               += $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
# policybench_LDADD               += $(top_builddir)/libxipc/libxipc.la
# policybench_LDADD               += $(top_builddir)/libcomm/libcomm.la
# policybench_LDADD               += $(top_builddir)/libxorp/libxorp.la

yacc:	.yacc_generated_files

.yacc_generated_files: $(srcdir)/compilepolicy.y
	( cd $(srcdir);							\
	yacc -d -p yy_compile_policy -o yacc.yy_compile_policy.cc compilepolicy.y; \
	$(top_srcdir)/utils/rcsid2ident.sh yacc.yy_compile_policy.cc;	\
	touch .yacc_generated_files )

lex:	.lex_generated_files

.lex_generated_files: $(srcdir)/compilepolicy.l
	( cd $(srcdir);							\
	flex -olex.yy_compile_policy.cc compilepolicy.l;		\
	touch .lex_generated_files )

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
