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

SUBDIRS = tests

#
# This is the only shared object that needs to be installed.
# By default it will be installed in directory @pkglibdir@ which
# in our case defaults to $(libdir)/xorp
# If we want to change the installation directory to something else,
# then we need to redefine pkglibdir. E.g.:
#pkglibdir=@libdir@
#
pkglib_PROGRAMS = libnetsnmpxorp.so


#
# These are the mib modules that will be loaded by the snmp agent.  They need
# not to be installed. Add the MIB modules that you write to this list.
# 
noinst_PROGRAMS = bgp4_mib_1657.so ospf_mib_1850.so xorp_if_mib_module.so

# -- Test Programs 
TESTS           = test_xorpevents
TESTS_ENVIRONMENT = LD_LIBRARY_PATH="$(top_builddir)/mibs"

noinst_PROGRAMS += $(TESTS)

# -- Compiler and pre compiler flags
AM_CPPFLAGS = `net-snmp-config --cflags` 

# -- Libraries and library flags to link the binary programs

# -Wl,--no-whole-archive must appear after the libraries, and that does not fit
# into the standard Automake link model.  This is why we must use the LINK
# directive
libnetsnmpxorp_so_LINK = $(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(AM_LDFLAGS) \
    $(LDFLAGS) -shared -fPIC -fpic -Wl,--whole-archive -o $@               \
    $(top_builddir)/libxipc/libxipc.a $(top_builddir)/libxorp/libxorp.a    \
    $(top_builddir)/libcomm/libcomm.a -Wl,--no-whole-archive
libnetsnmpxorp_so_DEPENDENCIES  = $(top_builddir)/libxipc/libxipc.a 
libnetsnmpxorp_so_DEPENDENCIES += $(top_builddir)/libxorp/libxorp.a
libnetsnmpxorp_so_DEPENDENCIES += $(top_builddir)/libcomm/libcomm.a

test_xorpevents_LDADD  = $(top_builddir)/libxipc/libfinder.a
test_xorpevents_LDADD += -lnetsnmpxorp -L${top_builddir}/mibs

bgp4_mib_1657_so_LDADD = -lnetsnmpxorp -L${top_builddir}/mibs
bgp4_mib_1657_so_LDADD += -lbgp4mibbase -L$(top_builddir)/xrl/targets
bgp4_mib_1657_so_LDADD += $(top_builddir)/xrl/interfaces/libbgpxif.a
bgp4_mib_1657_so_LDFLAGS = -shared -fPIC -fpic 
bgp4_mib_1657_so_DEPENDENCIES = libnetsnmpxorp.so

ospf_mib_1850_so_LDADD = -lnetsnmpxorp -L${top_builddir}/mibs  
ospf_mib_1850_so_LDFLAGS = -shared -fPIC -fpic 
ospf_mib_1850_so_DEPENDENCIES = libnetsnmpxorp.so

xorp_if_mib_module_so_LDADD = -lnetsnmpxorp -L${top_builddir}/mibs
xorp_if_mib_module_so_LDADD += -lxorpifmibbase -L$(top_builddir)/xrl/targets 
xorp_if_mib_module_so_LDFLAGS = -shared -fPIC -fpic 
xorp_if_mib_module_so_DEPENDENCIES = libnetsnmpxorp.so

# -- Test Program Sources

test_xorpevents_SOURCES  = test_xorpevents.cc

# -- Library Sources (libnetsnmpxorp)
libnetsnmpxorp_so_SOURCES  = xorpevents.cc
libnetsnmpxorp_so_SOURCES += xorpevents.hh
 
# -- Library Sources (bgp4_mib_1657)
bgp4_mib_1657_so_SOURCES  = bgp4_mib_1657.hh
bgp4_mib_1657_so_SOURCES += bgp4_mib_1657.cc
bgp4_mib_1657_so_SOURCES += bgp4_mib_1657_bgpversion.hh
bgp4_mib_1657_so_SOURCES += bgp4_mib_1657_bgpversion.cc
bgp4_mib_1657_so_SOURCES += bgp4_mib_1657_bgppeertable.hh
bgp4_mib_1657_so_SOURCES += bgp4_mib_1657_bgppeertable.cc
bgp4_mib_1657_so_SOURCES += bgp4_mib_xrl_target.cc


# -- Library Sources (ospf_mib_1850)
ospf_mib_1850_so_SOURCES  = ospf_mib_1850.hh
ospf_mib_1850_so_SOURCES += ospf_mib_1850.cc


# -- Library Sources (xorp_if_mib_module)
xorp_if_mib_module_so_SOURCES  = xorp_if_mib_module.hh
xorp_if_mib_module_so_SOURCES += xorp_if_mib_module.cc
xorp_if_mib_module_so_SOURCES += xorp_if_mib_xrl_target.cc
xorp_if_mib_module_so_SOURCES += xorp_if_mib_xrl_target.hh

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
