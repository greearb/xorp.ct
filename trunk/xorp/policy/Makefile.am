## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/Makefile.am,v 1.8 2004/09/17 19:41:24 pavlin Exp $
##

# -- Subdirectories
SUBDIRS = common backend . test

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS     = xorp_policy

# -- No-install Libraries
noinst_LTLIBRARIES = libpolicy.la

# -- Libraries and library flags to link the binary programs
LDADD            = $(noinst_LTLIBRARIES)
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
libpolicy_la_SOURCES		+= dependancy.hh
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
libpolicy_la_SOURCES		+= pw_notifier.hh
libpolicy_la_SOURCES		+= semantic_varrw.hh
libpolicy_la_SOURCES		+= set_map.hh
libpolicy_la_SOURCES		+= source_match_code_generator.hh
libpolicy_la_SOURCES		+= term.hh
libpolicy_la_SOURCES		+= var_map.hh
libpolicy_la_SOURCES		+= visitor.hh
libpolicy_la_SOURCES		+= visitor_semantic.hh
libpolicy_la_SOURCES		+= visitor_setdep.hh
libpolicy_la_SOURCES		+= xrl_target.hh

libpolicy_la_SOURCES		+= code.cc
libpolicy_la_SOURCES		+= code_generator.cc
libpolicy_la_SOURCES		+= code_list.cc
libpolicy_la_SOURCES		+= configuration.cc
libpolicy_la_SOURCES		+= export_code_generator.cc
libpolicy_la_SOURCES		+= filter_manager.cc
libpolicy_la_SOURCES		+= parser.cc
libpolicy_la_SOURCES		+= policy_list.cc
libpolicy_la_SOURCES		+= policy_map.cc
libpolicy_la_SOURCES		+= policy_statement.cc
libpolicy_la_SOURCES		+= process_watch.cc
libpolicy_la_SOURCES		+= semantic_varrw.cc
libpolicy_la_SOURCES		+= set_map.cc
libpolicy_la_SOURCES		+= source_match_code_generator.cc
libpolicy_la_SOURCES		+= term.cc
libpolicy_la_SOURCES		+= var_map.cc
libpolicy_la_SOURCES		+= visitor_semantic.cc
libpolicy_la_SOURCES		+= visitor_setdep.cc
libpolicy_la_SOURCES		+= xrl_target.cc


libpolicy_la_SOURCES		+= yacc.yy_policy_parser.h
libpolicy_la_SOURCES		+= yacc.yy_policy_parser.cc
libpolicy_la_SOURCES		+= lex.yy_policy_parser.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
