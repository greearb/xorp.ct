## Process this file with automake to produce Makefile.in.
##
## # $Header$
##

#
# This Makefile builds all the shared object files in this directory but
# without using libtool.  This forces us to use some hacks to build the
# libraries, such as declare them as programs, and override the linker
# default settings.
#

# --- Options
AUTOMAKE_OPTIONS	= foreign
ETAGS_ARGS		= --c++

SUBDIRS				 = snmpdscripts tests

#
# This is the only shared object that needs to be installed.
# By default it will be installed in directory @pkglibdir@ which
# in our case defaults to $(libdir)/xorp
# If we want to change the installation directory to something else,
# then we need to redefine pkglibdir. E.g.:
#pkglibdir=@libdir@
#

pkglib_LTLIBRARIES		 = libnetsnmpxorp.la

#
# These are the mib modules that will be loaded by the snmp agent.  They need
# not to be installed. Add the MIB modules that you write to this list.
# XXX: These can't be marked noinst if they are to be linked as shared
# libraries.
#
lib_LTLIBRARIES			 = bgp4_mib_1657.la
lib_LTLIBRARIES			+= ospf_mib_1850.la
lib_LTLIBRARIES			+= xorp_if_mib_module.la

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
