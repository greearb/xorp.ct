## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/harness/Makefile.am,v 1.2 2002/12/13 21:23:35 mjh Exp $
##

# -- Real Programs
bin_PROGRAMS	 = print_peers

# -- Test Programs
TESTS		 =

noinst_PROGRAMS  =
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD		+= $(top_builddir)/xrl/interfaces/libbgpxif.a

# -- Program Sources
print_peers_SOURCES = print_peer.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = $(TESTS)


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
