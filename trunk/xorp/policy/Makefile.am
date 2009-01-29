## Process this file with automake to produce Makefile.in.
#
## $XORP: xorp/policy/Makefile.am,v 1.17 2008/11/21 23:00:23 jtc Exp $
##

# -- Subdirectories
SUBDIRS = common backend .

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS     = xorp_policy

# -- Libraries
lib_LTLIBRARIES = libpolicy.la

# -- Libraries and library flags to link the binary programs
LDADD            = $(lib_LTLIBRARIES)
LDADD           += ./backend/libpolicybackend.la
LDADD           += ./common/libpolicycommon.la
LDADD           += $(top_builddir)/xrl/targets/libpolicybase.la
LDADD           += $(top_builddir)/xrl/interfaces/libpolicybackendxif.la
LDADD           += $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD           += $(top_builddir)/xrl/interfaces/libribxif.la
LDADD           += $(top_builddir)/libxipc/libxipc.la
LDADD           += $(top_builddir)/libcomm/libcomm.la
LDADD           += $(top_builddir)/libxorp/libxorp.la


# XORP Policy process
xorp_policy_SOURCES		 = xorp_policy.cc
xorp_policy_SOURCES		+= policy_target.hh
xorp_policy_SOURCES		+= policy_target.cc


# Policy library
libpolicy_la_SOURCES		 = policy_module.h
libpolicy_la_SOURCES		+= code.hh
libpolicy_la_SOURCES		+= code_generator.hh
libpolicy_la_SOURCES		+= code_list.hh
libpolicy_la_SOURCES		+= configuration.hh
libpolicy_la_SOURCES		+= dependency.hh
libpolicy_la_SOURCES		+= export_code_generator.hh
libpolicy_la_SOURCES		+= filter_manager.hh
libpolicy_la_SOURCES		+= filter_manager_base.hh
libpolicy_la_SOURCES		+= node.hh
libpolicy_la_SOURCES		+= node_base.hh
libpolicy_la_SOURCES		+= parser.hh
libpolicy_la_SOURCES		+= policy_list.hh
libpolicy_la_SOURCES		+= policy_map.hh
libpolicy_la_SOURCES		+= policy_parser.hh
libpolicy_la_SOURCES		+= policy_statement.hh
libpolicy_la_SOURCES		+= process_watch.hh
libpolicy_la_SOURCES		+= process_watch_base.hh
libpolicy_la_SOURCES		+= protocol_map.hh
libpolicy_la_SOURCES		+= pw_notifier.hh
libpolicy_la_SOURCES		+= semantic_varrw.hh
libpolicy_la_SOURCES		+= set_map.hh
libpolicy_la_SOURCES		+= source_match_code_generator.hh
libpolicy_la_SOURCES		+= term.hh
libpolicy_la_SOURCES		+= test_varrw.hh
libpolicy_la_SOURCES		+= var_map.hh
libpolicy_la_SOURCES		+= visitor.hh
libpolicy_la_SOURCES		+= visitor_dep.hh
libpolicy_la_SOURCES		+= visitor_printer.hh
libpolicy_la_SOURCES		+= visitor_semantic.hh
libpolicy_la_SOURCES		+= visitor_test.hh
libpolicy_la_SOURCES		+= xrl_target.hh

libpolicy_la_SOURCES		+= code.cc
libpolicy_la_SOURCES		+= code_generator.cc
libpolicy_la_SOURCES		+= code_list.cc
libpolicy_la_SOURCES		+= configuration.cc
libpolicy_la_SOURCES		+= dependency.cc
libpolicy_la_SOURCES		+= export_code_generator.cc
libpolicy_la_SOURCES		+= filter_manager.cc
libpolicy_la_SOURCES		+= parser.cc
libpolicy_la_SOURCES		+= policy_list.cc
libpolicy_la_SOURCES		+= policy_map.cc
libpolicy_la_SOURCES		+= policy_statement.cc
libpolicy_la_SOURCES		+= process_watch.cc
libpolicy_la_SOURCES		+= protocol_map.cc
libpolicy_la_SOURCES		+= semantic_varrw.cc
libpolicy_la_SOURCES		+= set_map.cc
libpolicy_la_SOURCES		+= source_match_code_generator.cc
libpolicy_la_SOURCES		+= term.cc
libpolicy_la_SOURCES		+= test_varrw.cc
libpolicy_la_SOURCES		+= var_map.cc
libpolicy_la_SOURCES		+= visitor_dep.cc
libpolicy_la_SOURCES		+= visitor_printer.cc
libpolicy_la_SOURCES		+= visitor_semantic.cc
libpolicy_la_SOURCES		+= visitor_test.cc
libpolicy_la_SOURCES		+= xrl_target.cc

libpolicy_la_SOURCES		+= yacc.yy_policy_parser.h
libpolicy_la_SOURCES		+= yacc.yy_policy_parser.cc
libpolicy_la_SOURCES		+= lex.yy_policy_parser.cc

libpolicy_la_LIBADD		 = $(top_builddir)/policy/backend/libpolicybackend.la
libpolicy_la_LIBADD		+= $(top_builddir)/policy/common/libpolicycommon.la
libpolicy_la_LIBADD		+= $(top_builddir)/xrl/targets/libpolicybase.la
libpolicy_la_LIBADD		+= $(top_builddir)/xrl/interfaces/libpolicybackendxif.la
libpolicy_la_LIBADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
libpolicy_la_LIBADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
libpolicy_la_LIBADD             += $(top_builddir)/libxipc/libxipc.la
libpolicy_la_LIBADD             += $(top_builddir)/libcomm/libcomm.la
libpolicy_la_LIBADD             += $(top_builddir)/libxorp/libxorp.la

yacc:	.yacc_generated_files

.yacc_generated_files: $(srcdir)/policy.y
	( cd $(srcdir);							\
	yacc -d -p yy_policy_parser -o yacc.yy_policy_parser.cc policy.y; \
	$(top_srcdir)/utils/rcsid2ident.sh yacc.yy_policy_parser.cc;	\
	touch .yacc_generated_files )

lex:	.lex_generated_files

.lex_generated_files: $(srcdir)/policy.l
	( cd $(srcdir);							\
	flex -olex.yy_policy_parser.cc policy.l;			\
	touch .lex_generated_files )

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
