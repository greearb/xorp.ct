## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/common/Makefile.am,v 1.2 2004/09/17 19:42:03 pavlin Exp $
##

# -- No-install Libraries
noinst_LTLIBRARIES = libpolicycommon.la

# Common policy library
libpolicycommon_la_SOURCES              = dispatcher.hh
libpolicycommon_la_SOURCES             += elem_null.hh
libpolicycommon_la_SOURCES             += elem_set.hh
libpolicycommon_la_SOURCES             += element.hh
libpolicycommon_la_SOURCES             += element_base.hh
libpolicycommon_la_SOURCES             += element_factory.hh
libpolicycommon_la_SOURCES             += filter.hh
libpolicycommon_la_SOURCES             += operator.hh
libpolicycommon_la_SOURCES             += operator_base.hh
libpolicycommon_la_SOURCES             += policy_exception.hh
libpolicycommon_la_SOURCES             += policy_utils.hh
libpolicycommon_la_SOURCES             += register_elements.hh
libpolicycommon_la_SOURCES             += register_operations.hh
libpolicycommon_la_SOURCES             += varrw.hh

libpolicycommon_la_SOURCES             += dispatcher.cc
libpolicycommon_la_SOURCES             += elem_set.cc
libpolicycommon_la_SOURCES             += element.cc
libpolicycommon_la_SOURCES             += element_factory.cc
libpolicycommon_la_SOURCES             += filter.cc
libpolicycommon_la_SOURCES             += policy_utils.cc
libpolicycommon_la_SOURCES             += register_elements.cc
libpolicycommon_la_SOURCES             += register_operations.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
