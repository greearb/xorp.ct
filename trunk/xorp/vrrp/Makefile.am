## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/vrrp/Makefile.am,v 1.5 2008/10/09 17:49:57 abittau Exp $
##

SUBDIRS = . test

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS     = xorp_vrrp

# -- No-install Libraries
noinst_LTLIBRARIES = libvrrp.la

# -- Libraries and library flags to link the binary programs
LDADD            = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/xrl/targets/libvrrpbase.la
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawlinkxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4xif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# XORP VRRP process
xorp_vrrp_SOURCES		 = xorp_vrrp.cc

# VRRP library
libvrrp_la_SOURCES		 = arpd.cc
libvrrp_la_SOURCES		+= vrrp.cc
libvrrp_la_SOURCES		+= vrrp_packet.cc
libvrrp_la_SOURCES		+= vrrp_target.cc
libvrrp_la_SOURCES		+= vrrp_vif.cc

libvrrp_la_SOURCES		+= arpd.hh
libvrrp_la_SOURCES		+= vrrp.hh
libvrrp_la_SOURCES		+= vrrp_exception.hh
libvrrp_la_SOURCES		+= vrrp_interface.hh
libvrrp_la_SOURCES		+= vrrp_module.h
libvrrp_la_SOURCES		+= vrrp_packet.hh
libvrrp_la_SOURCES		+= vrrp_target.hh
libvrrp_la_SOURCES		+= vrrp_vif.hh

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
