## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/tools/Makefile.am,v 1.2 2003/02/27 19:54:15 hodson Exp $
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
