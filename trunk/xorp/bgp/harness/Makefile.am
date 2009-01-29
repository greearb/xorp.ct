## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/bgp/harness/Makefile.am,v 1.24 2008/12/09 23:39:01 atanu Exp $
##

# -- Real Programs
bin_PROGRAMS	 =

# -- No-install Programs
check_PROGRAMS	 = coord
check_PROGRAMS	+= test_peer
check_PROGRAMS	+= test_trie

check_SCRIPTS	 = test_peering1.sh
check_SCRIPTS	+= test_peering2.sh
check_SCRIPTS	+= test_peering3.sh
check_SCRIPTS	+= test_routing1.sh
check_SCRIPTS	+= test_routing2.sh
check_SCRIPTS	+= test_rib1.sh
check_SCRIPTS	+= test_rib_fea1.sh
check_SCRIPTS	+= test_path_attribute1.sh
check_SCRIPTS	+= test_path_attribute2.sh
check_SCRIPTS	+= test_path_attribute3.sh
check_SCRIPTS	+= test_route_reflection1.sh
check_SCRIPTS	+= test_route_reflection2.sh
check_SCRIPTS	+= test_route_flap_damping1.sh
check_SCRIPTS	+= test_terminate.sh

TESTS		 = $(check_SCRIPTS) test_trie$(EXEEXT)

# -- Libraries
lib_LTLIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(lib_LTLIBRARIES)
LDADD		+= $(top_builddir)/bgp/libbgp.la
LDADD		+= $(top_builddir)/xrl/targets/libtestpeerbase.la
LDADD		+= $(top_builddir)/xrl/targets/libcoordbase.la
LDADD		+= $(top_builddir)/xrl/interfaces/libtestpeerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libdatainxif.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# LDADD		+= $(top_builddir)/xrl/targets/libbgpbase.la
# LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libbgpmibtrapsxif.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
# LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
# LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
# LDADD		+= $(top_builddir)/libxipc/libxipc.la
# LDADD		+= $(top_builddir)/libcomm/libcomm.la
# LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_peer_SOURCES = bgppp.hh test_peer.hh bgppp.cc ../dummy_main.cc test_peer.cc
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
coord_SOURCES	+= ../bgp.hh
coord_SOURCES	+= ../dummy_main.cc
test_trie_SOURCES =  test_trie.cc 
test_trie_SOURCES += trie.cc 
test_trie_SOURCES += trie.hh 
test_trie_SOURCES += real_trie.hh 
test_trie_SOURCES += trie_payload.hh
test_trie_SOURCES += ../dummy_main.cc

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

