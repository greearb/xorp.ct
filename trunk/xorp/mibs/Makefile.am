## Process this file with automake to produce Makefile.in.
##
## # $Header$
##



# - The shared objects
lib_LTLIBRARIES = libnetsnmpxorp.la

#noinst_LTLIBRARIES = bgp4_mib_1657.la ospf_mib_1850.la xorp_if_mib_module.la 

# -- Test Programs 
TESTS           = test_xorpevents

noinst_PROGRAMS = $(TESTS)

# -- Compiler and pre compiler flags
AM_CPPFLAGS = `net-snmp-config --cflags`

# -- Libraries and library flags to link the binary programs
#bgp4_mib_1657_LIBADD = libnetsnmpxorp.la
#ospf_mib_1850_LIBADD = libnetsnmpxorp.la
#xorp_if_mib_module_LIBADD = libnetsnmpxorp.la

libnetsnmpxorp_la_LIBADD  = $(top_builddir)/libxipc/libfinder.a
libnetsnmpxorp_la_LIBADD += $(top_builddir)/libxipc/libxipc.a
libnetsnmpxorp_la_LIBADD += $(top_builddir)/libxorp/libxorp.a
libnetsnmpxorp_la_LIBADD += $(top_builddir)/libcomm/libcomm.a
libnetsnmpxorp_la_LDFLAGS  = -version-info 0:0:0
 
test_xorpevents_LDADD  = libnetsnmpxorp.la
test_xorpevents_LDADD += $(top_builddir)/libxipc/libfinder.a
test_xorpevents_LDADD += $(top_builddir)/libxipc/libxipc.a
test_xorpevents_LDADD += $(top_builddir)/libxorp/libxorp.a
test_xorpevents_LDADD += $(top_builddir)/libcomm/libcomm.a

# The -module flag is required by libtool when the library name does not start
# with 'lib'
#bgp4_mib_1657_LDFLAGS = -module 
#ospf_mib_1850_LDFLAGS = -module 
#xorp_if_mib_module_LDFLAGS= -module 

# -- Test Program Sources

test_xorpevents_SOURCES  = test_xorpevents.cc

# -- Library Sources (libnetsnmpxorp)
libnetsnmpxorp_la_SOURCES  = xorpevents.cc
libnetsnmpxorp_la_SOURCES += xorpevents.hh
 
# -- Library Sources (bgp4_mib_1657)
#bgp4_mib_1657_SOURCES  = bgp4_mib_1657.hh
#bgp4_mib_1657_SOURCES += bgp4_mib_1657.cc
#bgp4_mib_1657_SOURCES += bgp4_mib_1657_bgpversion.hh
#bgp4_mib_1657_SOURCES += bgp4_mib_1657_bgpversion.cc

# -- Library Sources (ospf_mib_1850)
#ospf_mib_1850_SOURCES  = ospf_mib_1850.hh
#ospf_mib_1850_SOURCES += ospf_mib_1850.cc


# -- Library Sources (xorp_if_mib_module)
#xorp_if_mib_module_SOURCES  = xorp_if_mib_module.hh
#xorp_if_mib_module_SOURCES += xorp_if_mib_module.cc

# magic-script:			magic-script.in
# 	sed -e 's/whatever//' < $(srcdir)/magic-script.in > magic-script
# 	chmod +x magic-script


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##











