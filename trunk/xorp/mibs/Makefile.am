## Process this file with automake to produce Makefile.in.
##
## # $Header$
##

# All of the following shared objects will be installed in
# @pkglibdir@ rather than @libdir@ as they use the pkglib prefix.
# This can be changed by setting pkglibdir directly.

# --- Options
ETAGS_ARGS		= --c++

SUBDIRS				 = snmpdscripts tests

#
# libnetsnmpxorp contains the bindings which XORP MIB modules need to
# talk to the system's SNMP agent. It needs to be installed by default
# to support the MIB modules. It also brings in link-time dependencies
# on the XORP core libraries.
#
pkglib_LTLIBRARIES		 = libnetsnmpxorp.la
#
# These are the mib modules that will be loaded by the snmp agent. If
# we are configured --with-snmp, these will be installed at build-time.
# Add the MIB modules that you write to this list.
#
pkglib_LTLIBRARIES		+= bgp4_mib_1657.la
pkglib_LTLIBRARIES		+= ospf_mib_1850.la
pkglib_LTLIBRARIES		+= xorp_if_mib_module.la

# -- Test Programs

TESTS				 = test_xorpevents

check_PROGRAMS			 = $(TESTS)

# -- Compiler and pre compiler flags

AM_CPPFLAGS			 = `net-snmp-config --cflags` -UPACKAGE

# -- Linker flags for all targets in this directory

# -- Libraries and library flags to link the test programs

test_xorpevents_LDADD		 = ../libxipc/libfinder.la
test_xorpevents_LDADD		+= ./libnetsnmpxorp.la

# -- Libraries and library flags to link the libtoolized shared objects

libnetsnmpxorp_la_LIBADD	 = ../libxipc/libxipc.la
libnetsnmpxorp_la_LIBADD	+= ../libxorp/libxorp.la
libnetsnmpxorp_la_LIBADD	+= ../libcomm/libcomm.la
libnetsnmpxorp_la_LDFLAGS	 = -avoid-version

bgp4_mib_1657_la_LIBADD		 = ./libnetsnmpxorp.la
bgp4_mib_1657_la_LIBADD		+= ../xrl/targets/libbgp4mibbase.la
bgp4_mib_1657_la_LIBADD		+= ../xrl/interfaces/libbgpxif.la
bgp4_mib_1657_la_LDFLAGS	 = -module -avoid-version

ospf_mib_1850_la_LIBADD		 = ./libnetsnmpxorp.la
ospf_mib_1850_la_LDFLAGS	 = -module -avoid-version

xorp_if_mib_module_la_LIBADD	 = ./libnetsnmpxorp.la
xorp_if_mib_module_la_LIBADD	+= ../xrl/targets/libxorpifmibbase.la
xorp_if_mib_module_la_LDFLAGS	 = -module -avoid-version

# -- Test Program Sources
test_xorpevents_SOURCES		 = test_xorpevents.cc

# -- Library Sources (libnetsnmpxorp)
libnetsnmpxorp_la_SOURCES	 = xorp_netsnmp_module.h
libnetsnmpxorp_la_SOURCES	+= xorpevents.hh
libnetsnmpxorp_la_SOURCES	+= xorpevents.cc

# -- Library Sources (bgp4_mib_1657)
bgp4_mib_1657_la_SOURCES	 = bgp4_mib_1657.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgpversion.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgpversion.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgplocalas.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgplocalas.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgpidentifier.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgpidentifier.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgppeertable.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgppeertable.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgp4pathattrtable.hh
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_1657_bgp4pathattrtable.cc
bgp4_mib_1657_la_SOURCES	+= bgp4_mib_xrl_target.cc

# -- Library Sources (ospf_mib_1850)
ospf_mib_1850_la_SOURCES	 = ospf_mib_1850.hh
ospf_mib_1850_la_SOURCES	+= ospf_mib_1850.cc

# -- Library Sources (xorp_if_mib_module)
xorp_if_mib_module_la_SOURCES	 = xorp_if_mib_module.hh
xorp_if_mib_module_la_SOURCES	+= xorp_if_mib_module.cc
xorp_if_mib_module_la_SOURCES	+= xorp_if_mib_xrl_target.cc
xorp_if_mib_module_la_SOURCES	+= xorp_if_mib_xrl_target.hh

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES	 = -I$(top_srcdir)/.. -I$(top_builddir)/..

.NOTPARALLEL:
