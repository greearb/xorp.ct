## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/ospfd/xorp/Makefile.am,v 1.3 2003/03/31 16:10:18 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS = ospfd ospfd_mon ospfd_browser

# -- Test Programs
#bin_PROGRAMS +=

AM_CPPFLAGS  = -I$(top_builddir) -I$(srcdir)/../src

# -- Libraries and library flags to link the binary programs
LDADD	     = $(top_builddir)/ospfd/src/libospfd.a
LDADD	    += $(top_builddir)/xrl/targets/libospfbase.a
LDADD	    += $(top_builddir)/xrl/interfaces/libribxif.a
LDADD	    += $(top_builddir)/libxipc/libxipc.a
LDADD	    += $(top_builddir)/libcomm/libcomm.a
LDADD	    += $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
ospfd_SOURCES  = ospf_module.h
ospfd_SOURCES += ospfd_xorp.h
ospfd_SOURCES += os-instance.h
ospfd_SOURCES += tcppkt.h
ospfd_SOURCES += xrl_target.h
ospfd_SOURCES += 
ospfd_SOURCES += ospfd_xorp.C
ospfd_SOURCES += os-instance.C
ospfd_SOURCES += tcppkt.C
ospfd_SOURCES += system.C
ospfd_SOURCES += xrl_target.C

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

