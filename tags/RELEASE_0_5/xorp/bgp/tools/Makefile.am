## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/tools/Makefile.am,v 1.8 2003/05/07 21:58:07 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorpsh_print_peers xorpsh_print_routes

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  = print_peers print_routes

# -- No-install Libraries
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD		+= $(top_builddir)/xrl/interfaces/libbgpxif.a

# -- Program Sources
print_peers_SOURCES  = print_peer.cc
print_peers_SOURCES += print_peer.hh
print_peers_SOURCES += print_peers.cc

xorpsh_print_peers_SOURCES  = print_peer.cc
xorpsh_print_peers_SOURCES += print_peer.hh
xorpsh_print_peers_SOURCES += xorpsh_print_peers.cc

print_routes_SOURCES  = print_routes.cc 
print_routes_SOURCES += print_routes.hh 
print_routes_SOURCES += print_routes_main.cc 
print_routes_LDADD = $(LDADD)
print_routes_LDADD += $(top_builddir)/bgp/libbgp.a

xorpsh_print_routes_SOURCES  = print_routes.cc
xorpsh_print_routes_SOURCES += print_routes.hh
xorpsh_print_routes_SOURCES += xorpsh_print_routes.cc
xorpsh_print_routes_LDADD = $(LDADD)
xorpsh_print_routes_LDADD += $(top_builddir)/bgp/libbgp.a

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

