## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/Makefile.am,v 1.7 2004/09/17 13:48:47 abittau Exp $
##

# -- Subdirectories
SUBDIRS = common backend . test

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS     = xorp_policy

# -- No-install Libraries
noinst_LIBRARIES = libpolicy.a

# -- Libraries and library flags to link the binary programs
LDADD            = $(noinst_LIBRARIES)
LDADD           += ./common/libpolicycommon.a
LDADD           += $(top_builddir)/xrl/targets/libpolicybase.a
LDADD           += $(top_builddir)/xrl/interfaces/libpolicybackendxif.a
LDADD           += $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
LDADD           += $(top_builddir)/xrl/interfaces/libribxif.a
LDADD           += $(top_builddir)/libxipc/libxipc.a
LDADD           += $(top_builddir)/libcomm/libcomm.a
LDADD           += $(top_builddir)/libxorp/libxorp.a


# XORP Policy process
xorp_policy_SOURCES		 = xorp_policy.cc
xorp_policy_SOURCES		+= policy_target.hh
xorp_policy_SOURCES		+= policy_target.cc


# Policy library
libpolicy_a_SOURCES		 = policy_module.h
libpolicy_a_SOURCES		+= code.hh
libpolicy_a_SOURCES		+= code_generator.hh
libpolicy_a_SOURCES		+= code_list.hh
libpolicy_a_SOURCES		+= configuration.hh
libpolicy_a_SOURCES		+= dependancy.hh
libpolicy_a_SOURCES		+= export_code_generator.hh
libpolicy_a_SOURCES		+= filter_manager.hh
libpolicy_a_SOURCES		+= filter_manager_base.hh
libpolicy_a_SOURCES		+= node.hh
libpolicy_a_SOURCES		+= node_base.hh
libpolicy_a_SOURCES		+= parser.hh
libpolicy_a_SOURCES		+= policy_list.hh
libpolicy_a_SOURCES		+= policy_map.hh
libpolicy_a_SOURCES		+= policy_parser.hh
libpolicy_a_SOURCES		+= policy_statement.hh
libpolicy_a_SOURCES		+= process_watch.hh
libpolicy_a_SOURCES		+= process_watch_base.hh
libpolicy_a_SOURCES		+= pw_notifier.hh
libpolicy_a_SOURCES		+= semantic_varrw.hh
libpolicy_a_SOURCES		+= set_map.hh
libpolicy_a_SOURCES		+= source_match_code_generator.hh
libpolicy_a_SOURCES		+= term.hh
libpolicy_a_SOURCES		+= var_map.hh
libpolicy_a_SOURCES		+= visitor.hh
libpolicy_a_SOURCES		+= visitor_semantic.hh
libpolicy_a_SOURCES		+= visitor_setdep.hh
libpolicy_a_SOURCES		+= xrl_target.hh

libpolicy_a_SOURCES		+= code.cc
libpolicy_a_SOURCES		+= code_generator.cc
libpolicy_a_SOURCES		+= code_list.cc
libpolicy_a_SOURCES		+= configuration.cc
libpolicy_a_SOURCES		+= export_code_generator.cc
libpolicy_a_SOURCES		+= filter_manager.cc
libpolicy_a_SOURCES		+= parser.cc
libpolicy_a_SOURCES		+= policy_list.cc
libpolicy_a_SOURCES		+= policy_map.cc
libpolicy_a_SOURCES		+= policy_statement.cc
libpolicy_a_SOURCES		+= process_watch.cc
libpolicy_a_SOURCES		+= semantic_varrw.cc
libpolicy_a_SOURCES		+= set_map.cc
libpolicy_a_SOURCES		+= source_match_code_generator.cc
libpolicy_a_SOURCES		+= term.cc
libpolicy_a_SOURCES		+= var_map.cc
libpolicy_a_SOURCES		+= visitor_semantic.cc
libpolicy_a_SOURCES		+= visitor_setdep.cc
libpolicy_a_SOURCES		+= xrl_target.cc


libpolicy_a_SOURCES		+= yacc.yy_policy_parser.h
libpolicy_a_SOURCES		+= yacc.yy_policy_parser.cc
libpolicy_a_SOURCES		+= lex.yy_policy_parser.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
