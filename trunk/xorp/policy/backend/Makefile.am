##
## $XORP$
##

# -- No-install Libraries
noinst_LIBRARIES = libpolicybackend.a

# Backend policy library
libpolicybackend_a_SOURCES		 = instr_visitor.hh
libpolicybackend_a_SOURCES		+= instruction.hh
libpolicybackend_a_SOURCES		+= instruction_base.hh
libpolicybackend_a_SOURCES		+= iv_exec.hh
libpolicybackend_a_SOURCES		+= policy_backend_parser.hh
libpolicybackend_a_SOURCES		+= policy_filter.hh
libpolicybackend_a_SOURCES		+= policy_filters.hh
libpolicybackend_a_SOURCES		+= policy_instr.hh
libpolicybackend_a_SOURCES		+= policy_redist_map.hh
libpolicybackend_a_SOURCES		+= policytags.hh
libpolicybackend_a_SOURCES		+= set_manager.hh
libpolicybackend_a_SOURCES		+= single_varrw.hh
libpolicybackend_a_SOURCES		+= term_instr.hh

libpolicybackend_a_SOURCES		+= iv_exec.cc
libpolicybackend_a_SOURCES		+= policy_filter.cc
libpolicybackend_a_SOURCES		+= policy_filters.cc
libpolicybackend_a_SOURCES		+= policy_redist_map.cc
libpolicybackend_a_SOURCES		+= policytags.cc
libpolicybackend_a_SOURCES		+= set_manager.cc
libpolicybackend_a_SOURCES		+= single_varrw.cc


libpolicybackend_a_SOURCES		+= lex.yy_policy_backend_parser.cc
libpolicybackend_a_SOURCES		+= yacc.yy_policy_backend_parser.cc
libpolicybackend_a_SOURCES		+= yacc.yy_policy_backend_parser.h

# XXX find a way to link libpolicycommon
libpolicybackend_a_LIBADD		 = ../common/dispatcher.o
libpolicybackend_a_LIBADD		+= ../common/elem_set.o
libpolicybackend_a_LIBADD		+= ../common/element.o
libpolicybackend_a_LIBADD		+= ../common/element_factory.o
libpolicybackend_a_LIBADD		+= ../common/filter.o
libpolicybackend_a_LIBADD		+= ../common/policy_utils.o
libpolicybackend_a_LIBADD		+= ../common/register_elements.o
libpolicybackend_a_LIBADD		+= ../common/register_operations.o



INCLUDES =      -I$(top_srcdir)
