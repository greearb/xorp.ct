## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/harness/Makefile.am,v 1.11 2003/08/01 02:11:47 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_peering1.sh
TESTS		+= test_peering2.sh
TESTS		+= test_routing1.sh
TESTS		+= test_rib1.sh
TESTS		+= test_rib_fea1.sh
TESTS		+= test_path_attribute1.sh
TESTS		+= test_terminate.sh

# -- No-install Programs
noinst_PROGRAMS  = coord
noinst_PROGRAMS	+= test_peer

# -- No-install Libraries
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/bgp/libbgp.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD		+= $(top_builddir)/xrl/targets/libtestpeerbase.a
LDADD		+= $(top_builddir)/xrl/targets/libcoordbase.a
LDADD		+= $(top_builddir)/xrl/interfaces/libtestpeerxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libdatainxif.a

# -- Program Sources
test_peer_SOURCES = bgppp.hh test_peer.hh bgppp.cc test_peer.cc
coord_SOURCES	 = bgppp.hh
coord_SOURCES	+= command.hh
coord_SOURCES	+= coord.hh
coord_SOURCES	+= peer.hh
coord_SOURCES	+= trie.hh
coord_SOURCES	+= bgppp.cc
coord_SOURCES	+= command.cc
coord_SOURCES	+= coord.cc
coord_SOURCES	+= peer.cc
coord_SOURCES	+= trie.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = $(TESTS)
EXTRA_DIST	+= NOTES
EXTRA_DIST	+= args.sh
EXTRA_DIST	+= soak.sh
EXTRA_DIST	+= test1.sh
EXTRA_DIST	+= test2.sh
EXTRA_DIST	+= test_rib_fea1.sh
EXTRA_DIST	+= xrl_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

