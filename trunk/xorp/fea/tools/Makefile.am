## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/tools/Makefile.am,v 1.6 2003/08/01 02:11:51 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = show_interfaces

# -- Test Programs
TESTS		 = 

# -- No-install Programs
noinst_PROGRAMS  = 

# -- No-install Libraries
noinst_LIBRARIES = 

# -- Libraries and library flags to link the binary programs
LDADD		 = $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
show_interfaces_SOURCES	 = show_interfaces.hh
show_interfaces_SOURCES	+= show_interfaces.cc
show_interfaces_LDADD	 = $(top_builddir)/libfeaclient/libfeaclient.a
show_interfaces_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
show_interfaces_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
show_interfaces_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
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

