## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/contrib/ospfd/xorp/Makefile.am,v 1.9 2004/06/10 16:54:47 hodson Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_ospf

# -- Test Programs
TESTS		 = 

# -- No-install Programs
noinst_PROGRAMS	 = ospfd_mon
noinst_PROGRAMS	+= ospfd_browser

# -- No-install Libraries
noinst_LTLIBRARIES = 

AM_CPPFLAGS  = -I$(top_builddir) -I$(srcdir)/../src

# -- Libraries and library flags to link the binary programs
LDADD	     = $(top_builddir)/contrib/ospfd/src/libospfd.la
LDADD	    += $(top_builddir)/xrl/targets/libospfbase.la
LDADD	    += $(top_builddir)/xrl/interfaces/libribxif.la
LDADD	    += $(top_builddir)/libxipc/libxipc.la
LDADD	    += $(top_builddir)/libcomm/libcomm.la
LDADD	    += $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_ospf_SOURCES  = ospf_module.h
xorp_ospf_SOURCES += ospfd_xorp.h
xorp_ospf_SOURCES += os-instance.h
xorp_ospf_SOURCES += tcppkt.h
xorp_ospf_SOURCES += xrl_target.h
xorp_ospf_SOURCES += 
xorp_ospf_SOURCES += ospfd_xorp.C
xorp_ospf_SOURCES += os-instance.C
xorp_ospf_SOURCES += tcppkt.C
xorp_ospf_SOURCES += system.C
xorp_ospf_SOURCES += xrl_target.C

ospfd_mon_SOURCES  = tcppkt.h
ospfd_mon_SOURCES += tcppkt.C
ospfd_mon_SOURCES += lsa_prn.C
ospfd_mon_SOURCES += ospfd_mon.C

ospfd_browser_SOURCES  = tcppkt.h
ospfd_browser_SOURCES += tcppkt.C
ospfd_browser_SOURCES += lsa_prn.C
ospfd_browser_SOURCES += ospfd_browser.C

# -- Library Sources


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

