## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# -- No-install Libraries
noinst_LIBRARIES = libpolicycommon.a

# Common policy library
libpolicycommon_a_SOURCES              = dispatcher.hh
libpolicycommon_a_SOURCES             += elem_null.hh
libpolicycommon_a_SOURCES             += elem_set.hh
libpolicycommon_a_SOURCES             += element.hh
libpolicycommon_a_SOURCES             += element_base.hh
libpolicycommon_a_SOURCES             += element_factory.hh
libpolicycommon_a_SOURCES             += filter.hh
libpolicycommon_a_SOURCES             += operator.hh
libpolicycommon_a_SOURCES             += operator_base.hh
libpolicycommon_a_SOURCES             += policy_exception.hh
libpolicycommon_a_SOURCES             += policy_utils.hh
libpolicycommon_a_SOURCES             += register_elements.hh
libpolicycommon_a_SOURCES             += register_operations.hh
libpolicycommon_a_SOURCES             += varrw.hh

libpolicycommon_a_SOURCES             += dispatcher.cc
libpolicycommon_a_SOURCES             += elem_set.cc
libpolicycommon_a_SOURCES             += element.cc
libpolicycommon_a_SOURCES             += element_factory.cc
libpolicycommon_a_SOURCES             += filter.cc
libpolicycommon_a_SOURCES             += policy_utils.cc
libpolicycommon_a_SOURCES             += register_elements.cc
libpolicycommon_a_SOURCES             += register_operations.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
