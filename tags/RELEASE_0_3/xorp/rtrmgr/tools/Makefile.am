## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/tools/Makefile.am,v 1.3 2003/03/31 16:10:18 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS	 = show_interfaces

# -- Test Programs
#bin_PROGRAMS	+=

#TESTS		 = 

#noinst_PROGRAMS = $(TESTS)

#EXTRA_DIST	 = 

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

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

