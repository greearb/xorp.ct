## Process this file with automake to produce Makefile.in.
##
## # $Header$
##

# This makefile builds all the shared object files in this directory but without
# using libtool.  This forces us to use some hacks to build the libraries, such
# as declare them as programs, and override the linker default settings.

# This is the only shared object that needs to be installed (manually, since we
# are not using libtool)
bin_PROGRAMS = libnetsnmpxorp.so

# These are the mib modules that will be loaded by the snmp agent.  They need
# not to be installed. Add the MIB modules that you write to this list.
# 
# XXX: using the prefix noinst_ would be preferred, but automake complains...
sbin_PROGRAMS = bgp4_mib_1657.so ospf_mib_1850.so xorp_if_mib_module.so

# -- Test Programs 
TESTS           = test_xorpevents

noinst_PROGRAMS = $(TESTS)

# -- Compiler and pre compiler flags
AM_CPPFLAGS = `net-snmp-config --cflags` 

# -- Libraries and library flags to link the binary programs

LDADD  = $(top_builddir)/libxipc/libfinder.a
LDADD += $(top_builddir)/libxipc/libxipc.a
LDADD += $(top_builddir)/libxorp/libxorp.a
LDADD += $(top_builddir)/libcomm/libcomm.a
 
libnetsnmpxorp_so_LDFLAGS = -shared -fPIC -fpic 

test_xorpevents_LDADD  = $(LDADD) -lnetsnmpxorp -L.

bgp4_mib_1657_so_LDADD = $(LDADD) -lnetsnmpxorp -L.
bgp4_mib_1657_so_LDADD += $(top_builddir)/xrl/interfaces/libbgpxif.a
bgp4_mib_1657_so_LDFLAGS = -shared -fPIC -fpic 
 

ospf_mib_1850_so_LDADD = $(LDADD) -lnetsnmpxorp -L.
ospf_mib_1850_so_LDFLAGS = -shared -fPIC -fpic 

xorp_if_mib_module_so_LDADD = $(LDADD) -lnetsnmpxorp -L.
xorp_if_mib_module_so_LDFLAGS = -shared -fPIC -fpic 


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

# -- Library Sources (ospf_mib_1850)
ospf_mib_1850_so_SOURCES  = ospf_mib_1850.hh
ospf_mib_1850_so_SOURCES += ospf_mib_1850.cc


# -- Library Sources (xorp_if_mib_module)
xorp_if_mib_module_so_SOURCES  = xorp_if_mib_module.hh
xorp_if_mib_module_so_SOURCES += xorp_if_mib_module.cc

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##











