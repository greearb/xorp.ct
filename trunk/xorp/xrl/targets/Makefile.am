##
## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/targets/Makefile.am,v 1.9 2003/05/16 16:52:09 hodson Exp $
##

INTERFACES_DIR=$(top_srcdir)/xrl/interfaces
TARGET_DIR=$(top_srcdir)/xrl/targets


###############################################################################
# Xrl Target related
###############################################################################

# Add your target's library here
noinst_LIBRARIES  = libbgpbase.a
noinst_LIBRARIES += libbgp4mibbase.a
noinst_LIBRARIES += libclibase.a
noinst_LIBRARIES += libcoordbase.a
noinst_LIBRARIES += libdemofeaifmgrclientbase.a
noinst_LIBRARIES += libfeabase.a
noinst_LIBRARIES += libfinderbase.a
noinst_LIBRARIES += libfinderclientbase.a
noinst_LIBRARIES += libmfeabase.a
noinst_LIBRARIES += libmld6igmpbase.a
noinst_LIBRARIES += libospfbase.a
noinst_LIBRARIES += libpimbase.a
noinst_LIBRARIES += libribbase.a
noinst_LIBRARIES += libribclientbase.a
noinst_LIBRARIES += librtrmgrbase.a
noinst_LIBRARIES += libtestbase.a
noinst_LIBRARIES += libtestfindereventsbase.a
noinst_LIBRARIES += libtestpeerbase.a
noinst_LIBRARIES += libxorpshbase.a

# BGP
libbgpbase_a_SOURCES = bgp_base.hh bgp_base.cc
$(srcdir)/bgp_base.hh $(srcdir)/bgp_base.cc:				\
		$(TARGET_DIR)/bgp.tgt					\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/bgp.xif				\
		$(INTERFACES_DIR)/rib_client.xif

# BGP4MIB
libbgp4mibbase_a_SOURCES = bgp4_mib_base.hh bgp4_mib_base.cc
$(srcdir)/bgp4_mib_base.hh $(srcdir)/bgp4_mib_base.cc:			\
		$(INTERFACES_DIR)/common.xif

# CLI
libclibase_a_SOURCES = cli_base.hh cli_base.cc
$(srcdir)/cli_base.hh $(srcdir)/cli_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/cli_manager.xif

# COORD
libcoordbase_a_SOURCES = coord_base.hh coord_base.cc
$(srcdir)/coord_base.hh $(srcdir)/coord_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/coord.xif				\
		$(INTERFACES_DIR)/datain.xif

# Demo FEA ifmgr client
libdemofeaifmgrclientbase_a_SOURCES = demo_fea_ifmgr_client_base.hh	\
				      demo_fea_ifmgr_client_base.cc

$(srcdir)/demo_fea_ifmgr_client_base.hh					\
$(srcdir)/demo_fea_ifmgr_client_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_ifmgr_client.xif

# FEA
libfeabase_a_SOURCES = fea_base.hh fea_base.cc
$(srcdir)/fea_base.hh $(srcdir)/fea_base.cc: 				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_ifmgr.xif				\
		$(INTERFACES_DIR)/fti.xif				\
		$(INTERFACES_DIR)/fea_rawpkt.xif

# Finder
libfinderbase_a_SOURCES = finder_base.hh finder_base.cc
$(srcdir)/finder_base.hh $(srcdir)/finder_base.cc : 			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder.xif				\
		$(INTERFACES_DIR)/finder_event_notifier.xif

# Finder client
libfinderclientbase_a_SOURCES = finder_client_base.hh finder_client_base.cc
$(srcdir)/finder_client_base.hh $(srcdir)/finder_client_base.cc: 	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_client.xif

# MFEA
libmfeabase_a_SOURCES = mfea_base.hh mfea_base.cc
$(srcdir)/mfea_base.hh $(srcdir)/mfea_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif			\
		$(INTERFACES_DIR)/mfea.xif				\
		$(INTERFACES_DIR)/fea_ifmgr_client.xif

# MLD6IGMP
libmld6igmpbase_a_SOURCES = mld6igmp_base.hh mld6igmp_base.cc
$(srcdir)/mld6igmp_base.hh $(srcdir)/mld6igmp_base.cc: 			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif 			\
		$(INTERFACES_DIR)/mfea_client.xif			\
		$(INTERFACES_DIR)/mld6igmp.xif

# OSPF
libospfbase_a_SOURCES = ospf_base.hh ospf_base.cc
$(srcdir)/ospf_base.hh $(srcdir)/ospf_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/ospf.xif

# PIM
libpimbase_a_SOURCES = pim_base.hh pim_base.cc
$(srcdir)/pim_base.hh $(srcdir)/pim_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif			\
		$(INTERFACES_DIR)/mfea_client.xif			\
		$(INTERFACES_DIR)/mld6igmp_client.xif			\
		$(INTERFACES_DIR)/fti.xif				\
		$(INTERFACES_DIR)/pim.xif

# RIB
libribbase_a_SOURCES = rib_base.hh rib_base.cc
$(srcdir)/rib_base.hh $(srcdir)/rib_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rib.xif

# RIB client
libribclientbase_a_SOURCES = ribclient_base.hh ribclient_base.cc
$(srcdir)/ribclient_base.hh $(srcdir)/ribclient_base.cc: 		\
		$(INTERFACES_DIR)/rib_client.xif

# RTRMGR
librtrmgrbase_a_SOURCES = rtrmgr_base.hh rtrmgr_base.cc
$(srcdir)/rtrmgr_base.hh $(srcdir)/rtrmgr_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr.xif

# Test finder events
libtestfindereventsbase_a_SOURCES  = test_finder_events_base.hh
libtestfindereventsbase_a_SOURCES += test_finder_events_base.cc
$(srcdir)/test_finder_events_base.hh $(srcdir)/test_finder_events_base.hh: \
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif

# TEST PEER
libtestpeerbase_a_SOURCES = test_peer_base.hh test_peer_base.cc
$(srcdir)/test_peer_base.hh $(srcdir)/test_peer_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_peer.xif

# Xorpsh
libxorpshbase_a_SOURCES = xorpsh_base.hh xorpsh_base.cc
$(srcdir)/xorpsh_base.hh $(srcdir)/xorpsh_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr_client.xif

# Add your target's library sources like this:
# Test target
libtestbase_a_SOURCES = test_base.hh test_base.cc
# This is a hack to rebuild files here when interfaces implemented by
# rib target are touched.  It largely duplicates information inside rib.tgt
$(srcdir)/test_base.hh $(srcdir)/test_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test.xif

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

###############################################################################
# Implicit Rules and related
###############################################################################

SCRIPT_DIR=$(top_srcdir)/xrl/scripts
TGTGEN_PY=$(SCRIPT_DIR)/tgt-gen

# If this code is commented out, please upgrade to python2.0 or above.

@PY_BUILD@$(srcdir)/%_base.hh $(srcdir)/%_base.cc %_base.hh %_base.cc	\
@PY_BUILD@$(srcdir)/%.xrls: $(srcdir)/%.tgt $(TGTGEN_PY)
@PY_BUILD@	$(PYTHON) $(TGTGEN_PY) -I$(INTERFACES_DIR) $<


