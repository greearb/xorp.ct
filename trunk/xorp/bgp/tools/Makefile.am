## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/tools/Makefile.am,v 1.3 2003/02/03 23:40:28 mjh Exp $
##

# -- Real Programs
bin_PROGRAMS	 = print_peers xorpsh_print_peers print_routes

noinst_PROGRAMS  =
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD		+= $(top_builddir)/xrl/interfaces/libbgpxif.a

# -- Program Sources
print_peers_SOURCES = print_peer.cc print_peer.hh print_peers.cc
xorpsh_print_peers_SOURCES = print_peer.cc print_peer.hh xorpsh_print_peers.cc

print_routes_SOURCES = print_routes.cc 
print_routes_LDADD = $(LDADD)
print_routes_LDADD += $(top_builddir)/bgp/libbgp.a

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
