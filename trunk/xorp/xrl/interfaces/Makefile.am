## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/interfaces/Makefile.am,v 1.1.1.1 2002/12/11 23:56:17 hodson Exp $
##

###############################################################################
# Client Interface related
###############################################################################

# BGP Client Interface 
noinst_LIBRARIES = libbgpxif.a
libbgpxif_a_SOURCES = bgp_xif.hh bgp_xif.cc

# Command-Line Interface Manager Interface
noinst_LIBRARIES += libclimanagerxif.a
libclimanagerxif_a_SOURCES = cli_manager_xif.hh cli_manager_xif.cc

# Command-Line Interface Processor Interface
noinst_LIBRARIES += libcliprocessorxif.a
libcliprocessorxif_a_SOURCES = cli_processor_xif.hh cli_processor_xif.cc

# Common Client Interface
noinst_LIBRARIES  += libcommonxif.a
libcommonxif_a_SOURCES = common_xif.hh common_xif.cc

# Interface to the coordinating BGP test process
noinst_LIBRARIES += libcoordxif.a
libcoordxif_a_SOURCES = coord_xif.hh coord_xif.cc

# Datain client of the test peer interface.
noinst_LIBRARIES += libdatainxif.a
libdatainxif_a_SOURCES = datain_xif.hh datain_xif.cc

# FEA Client Interface
noinst_LIBRARIES += libfeaifmgrxif.a
libfeaifmgrxif_a_SOURCES = fea_ifmgr_xif.hh fea_ifmgr_xif.cc

# FEA Interface Manager Client Interface
noinst_LIBRARIES += libfeaifmgrclientxif.a
libfeaifmgrclientxif_a_SOURCES  = fea_ifmgr_client_xif.hh 
libfeaifmgrclientxif_a_SOURCES += fea_ifmgr_client_xif.cc 

# Forwarding Table Interface Client Interface
noinst_LIBRARIES += libftixif.a
libftixif_a_SOURCES = fti_xif.hh fti_xif.cc

# FEA Raw Packet Server Interface
noinst_LIBRARIES += libfearawpkt.a
libfearawpkt_a_SOURCES  = fea_rawpkt_xif.hh
libfearawpkt_a_SOURCES += fea_rawpkt_xif.cc

# FEA Raw Packet Client Interface
noinst_LIBRARIES += libfearawpktclient.a
libfearawpktclient_a_SOURCES  = fea_rawpkt_client_xif.hh 
libfearawpktclient_a_SOURCES += fea_rawpkt_client_xif.cc 

# Finder Interface
noinst_LIBRARIES += libfinderxif.a
libfinderxif_a_SOURCES = finder_xif.hh finder_xif.cc

# Finder Client Interface (client of Finder)
noinst_LIBRARIES += libfinderclientxif.a
libfinderclientxif_a_SOURCES = finder_client_xif.hh finder_client_xif.cc

# Multicast FEA Interface
noinst_LIBRARIES += libmfeaxif.a
libmfeaxif_a_SOURCES = mfea_xif.hh mfea_xif.cc

# Multicast FEA Client Interface
noinst_LIBRARIES += libmfeaclientxif.a
libmfeaclientxif_a_SOURCES = mfea_client_xif.hh mfea_client_xif.cc

# MLD6/IGMP Interface
noinst_LIBRARIES += libmld6igmpxif.a
libmld6igmpxif_a_SOURCES = mld6igmp_xif.hh mld6igmp_xif.cc

# MLD6/IGMP Client Interface
noinst_LIBRARIES += libmld6igmpclientxif.a
libmld6igmpclientxif_a_SOURCES = mld6igmp_client_xif.hh mld6igmp_client_xif.cc

# OSPF Interface
noinst_LIBRARIES += libospfxif.a
libospfxif_a_SOURCES = ospf_xif.hh ospf_xif.cc

# PIM Interface
noinst_LIBRARIES += libpimxif.a
libpimxif_a_SOURCES = pim_xif.hh pim_xif.cc

# RIB Interface to Routing Protocols
noinst_LIBRARIES += libribxif.a
libribxif_a_SOURCES = rib_xif.hh rib_xif.cc

# RIB Client Interface supported by BGP
noinst_LIBRARIES += libribclientxif.a
libribclientxif_a_SOURCES = rib_client_xif.hh rib_client_xif.cc

# RTRMGR Interface
noinst_LIBRARIES += librtrmgrxif.a
librtrmgrxif_a_SOURCES = rtrmgr_xif.hh rtrmgr_xif.cc

# RTRMGR Client Interface
noinst_LIBRARIES += librtrmgr_clientxif.a
librtrmgr_clientxif_a_SOURCES = rtrmgr_client_xif.hh rtrmgr_client_xif.cc

# Test Peer Interface
noinst_LIBRARIES += libtestpeerxif.a
libtestpeerxif_a_SOURCES = test_peer_xif.hh test_peer_xif.cc


#
# Sample Test Client Interface 
#
noinst_LIBRARIES += libtestxif.a
libtestxif_a_SOURCES = test_xif.hh test_xif.cc


###############################################################################
# Implicit Rules
###############################################################################

SCRIPT_DIR=../scripts
CLNTGEN_PY=$(SCRIPT_DIR)/clnt-gen

@PY_BUILD@%_xif.hh %_xif.cc: %.xif $(CLNTGEN_PY)
@PY_BUILD@	$(PYTHON) $(CLNTGEN_PY) $<
