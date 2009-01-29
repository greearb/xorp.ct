## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/tools/Makefile.am,v 1.2 2004/11/12 02:44:40 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = show_interfaces

# -- Test Programs
TESTS		 = 

# -- No-install Programs
noinst_PROGRAMS  = 

# -- Libraries
lib_LTLIBRARIES = 

# -- Libraries and library flags to link the binary programs
LDADD		 = $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
show_interfaces_SOURCES	 = show_interfaces.hh
show_interfaces_SOURCES	+= show_interfaces.cc
show_interfaces_LDADD	 = $(top_builddir)/libfeaclient/libfeaclient.la
show_interfaces_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
show_interfaces_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
show_interfaces_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
show_interfaces_LDADD	+= $(LDADD)

# -- Library Sources

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = 

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

