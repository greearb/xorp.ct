## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/backend/Makefile.am,v 1.2 2004/09/17 19:42:02 pavlin Exp $
##

# -- No-install Libraries
noinst_LTLIBRARIES = libpolicybackend.la

# Backend policy library
libpolicybackend_la_SOURCES		 = instr_visitor.hh
libpolicybackend_la_SOURCES		+= instruction.hh
libpolicybackend_la_SOURCES		+= instruction_base.hh
libpolicybackend_la_SOURCES		+= iv_exec.hh
libpolicybackend_la_SOURCES		+= policy_backend_parser.hh
libpolicybackend_la_SOURCES		+= policy_filter.hh
libpolicybackend_la_SOURCES		+= policy_filters.hh
libpolicybackend_la_SOURCES		+= policy_instr.hh
libpolicybackend_la_SOURCES		+= policy_redist_map.hh
libpolicybackend_la_SOURCES		+= policytags.hh
libpolicybackend_la_SOURCES		+= set_manager.hh
libpolicybackend_la_SOURCES		+= single_varrw.hh
libpolicybackend_la_SOURCES		+= term_instr.hh

libpolicybackend_la_SOURCES		+= iv_exec.cc
libpolicybackend_la_SOURCES		+= policy_filter.cc
libpolicybackend_la_SOURCES		+= policy_filters.cc
libpolicybackend_la_SOURCES		+= policy_redist_map.cc
libpolicybackend_la_SOURCES		+= policytags.cc
libpolicybackend_la_SOURCES		+= set_manager.cc
libpolicybackend_la_SOURCES		+= single_varrw.cc


libpolicybackend_la_SOURCES		+= lex.yy_policy_backend_parser.cc
libpolicybackend_la_SOURCES		+= yacc.yy_policy_backend_parser.cc
libpolicybackend_la_SOURCES		+= yacc.yy_policy_backend_parser.h

# XXX find a way to link libpolicycommon
libpolicybackend_la_LIBADD		 = ../common/dispatcher.lo
libpolicybackend_la_LIBADD		+= ../common/elem_set.lo
libpolicybackend_la_LIBADD		+= ../common/element.lo
libpolicybackend_la_LIBADD		+= ../common/element_factory.lo
libpolicybackend_la_LIBADD		+= ../common/filter.lo
libpolicybackend_la_LIBADD		+= ../common/policy_utils.lo
libpolicybackend_la_LIBADD		+= ../common/register_elements.lo
libpolicybackend_la_LIBADD		+= ../common/register_operations.lo


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
