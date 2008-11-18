## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/examples/usermgr/Makefile.am,v 1.2 2008/10/29 22:24:14 paulz Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS     = xorp_usermgr

# -- No-install programs
check_PROGRAMS = test_usermgr

# -- No-install Libraries
noinst_LTLIBRARIES = libusermgr.la

# -- Libraries and library flags to link the binary programs
LDADD            = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/xrl/targets/libusermgrbase.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# XORP USERMGR process
xorp_usermgr_SOURCES		 = xorp_usermgr.cc

# test_usermgr sources
test_usermgr_SOURCES 		 = test_usermgr.cc

# USERMGR library
libusermgr_la_SOURCES		 = usermgr.cc
libusermgr_la_SOURCES		+=  xrl_target.cc

libusermgr_la_SOURCES		+= usermgr.hh
libusermgr_la_SOURCES		+= usermgr_exception.hh
libusermgr_la_SOURCES		+= usermgr_module.h
libusermgr_la_SOURCES		+= xrl_target.hh

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
