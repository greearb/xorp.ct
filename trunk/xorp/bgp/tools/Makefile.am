## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/tools/Makefile.am,v 1.2 2003/01/21 19:21:19 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS	 = print_peers print_routes

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

print_routes_SOURCES = print_routes.cc 
print_routes_LDADD = $(LDADD)
print_routes_LDADD += $(top_builddir)/bgp/libbgp.a

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
