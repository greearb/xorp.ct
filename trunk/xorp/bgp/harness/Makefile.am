## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/harness/Makefile.am,v 1.15 2004/11/12 02:44:28 bms Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_trie
TESTS		+= test_peering1.sh
TESTS		+= test_peering2.sh
TESTS		+= test_routing1.sh
TESTS		+= test_routing2.sh
TESTS		+= test_rib1.sh
TESTS		+= test_rib_fea1.sh
TESTS		+= test_path_attribute1.sh
TESTS		+= test_terminate.sh

# -- No-install Programs
check_PROGRAMS  = coord
check_PROGRAMS	+= test_peer
check_PROGRAMS	+= test_trie

# -- No-install Libraries
noinst_LTLIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/bgp/libbgp.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la
LDADD		+= $(top_builddir)/xrl/targets/libtestpeerbase.la
LDADD		+= $(top_builddir)/xrl/targets/libcoordbase.la
LDADD		+= $(top_builddir)/xrl/interfaces/libtestpeerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libdatainxif.la

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
coord_SOURCES	+= real_trie.hh
coord_SOURCES	+= trie_payload.hh
test_trie_SOURCES = test_trie.cc trie.cc trie.hh real_trie.hh trie_payload.hh

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

