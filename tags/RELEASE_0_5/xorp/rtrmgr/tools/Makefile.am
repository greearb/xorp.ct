## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/tools/Makefile.am,v 1.5 2003/07/30 23:43:32 pavlin Exp $
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
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a

# -- Program Sources
show_interfaces_SOURCES	 = show_interfaces.hh
show_interfaces_SOURCES	+= show_interfaces.cc

# -- Library Sources

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = 

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

