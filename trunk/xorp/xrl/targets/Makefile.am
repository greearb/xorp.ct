##
## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/targets/Makefile.am,v 1.6 2003/05/13 00:21:03 hodson Exp $
##

INTERFACES_DIR=$(top_srcdir)/xrl/interfaces

###############################################################################
# Xrl Target related
###############################################################################

# Add your target's library here
noinst_LIBRARIES  = libbgpbase.a
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
noinst_LIBRARIES += libtestpeerbase.a
noinst_LIBRARIES += libxorpshbase.a
noinst_LIBRARIES += libbgp4mibbase.a

# BGP
libbgpbase_a_SOURCES = bgp_base.hh bgp_base.cc
bgp_base.hh: $(INTERFACES_DIR)/common.xif				\
	$(INTERFACES_DIR)/bgp.xif					\
	$(INTERFACES_DIR)/rib_client.xif

# CLI
libclibase_a_SOURCES = cli_base.hh cli_base.cc
cli_base.hh: $(INTERFACES_DIR)/common.xif $(INTERFACES_DIR)/cli_manager.xif

# COORD
libcoordbase_a_SOURCES = coord_base.hh coord_base.cc
coord_base.hh: $(INTERFACES_DIR)/common.xif				\
	       $(INTERFACES_DIR)/coord.xif 				\
	       $(INTERFACES_DIR)/datain.xif

# Demo FEA ifmgr client
libdemofeaifmgrclientbase_a_SOURCES = demo_fea_ifmgr_client_base.hh	\
				      demo_fea_ifmgr_client_base.cc
demo_fea_if_client_base.hh: $(INTERFACES_DIR)/common.xif 		\
			    $(INTERFACES_DIR)/fea_ifmgr_client.xif

# FEA
libfeabase_a_SOURCES = fea_base.hh fea_base.cc
fea_base.hh: $(INTERFACES_DIR)/common.xif 				\
	     $(INTERFACES_DIR)/fea_ifmgr.xif				\
             $(INTERFACES_DIR)/fti.xif					\
	     $(INTERFACES_DIR)/fea_rawpkt.xif

# Finder
libfinderbase_a_SOURCES = finder_base.hh finder_base.cc
finder_base.hh: $(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder.xif				\
		$(INTERFACES_DIR)/finder_event_notifier.xif

# Finder client
libfinderclientbase_a_SOURCES = finder_client_base.hh finder_client_base.cc
finder_client_base.hh: $(INTERFACES_DIR)/common.xif 			\
                       $(INTERFACES_DIR)/finder_client.xif

# MFEA
libmfeabase_a_SOURCES = mfea_base.hh mfea_base.cc
mfea_base.hh: $(INTERFACES_DIR)/common.xif 				\
              $(INTERFACES_DIR)/cli_processor.xif                       \
              $(INTERFACES_DIR)/mfea.xif

# MLD6IGMP
libmld6igmpbase_a_SOURCES = mld6igmp_base.hh mld6igmp_base.cc
mld6igmp_base.hh: $(INTERFACES_DIR)/common.xif 				\
                  $(INTERFACES_DIR)/cli_processor.xif 			\
	          $(INTERFACES_DIR)/mfea_client.xif			\
                  $(INTERFACES_DIR)/mld6igmp.xif

# OSPF
libospfbase_a_SOURCES = ospf_base.hh ospf_base.cc
ospf_base.hh: $(INTERFACES_DIR)/common.xif $(INTERFACES_DIR)/ospf.xif

# PIM
libpimbase_a_SOURCES = pim_base.hh pim_base.cc
pim_base.hh: $(INTERFACES_DIR)/common.xif 				\
             $(INTERFACES_DIR)/cli_processor.xif			\
	     $(INTERFACES_DIR)/mfea_client.xif				\
	     $(INTERFACES_DIR)/mld6igmp_client.xif			\
             $(INTERFACES_DIR)/fti.xif					\
	     $(INTERFACES_DIR)/pim.xif

# RIB
libribbase_a_SOURCES = rib_base.hh rib_base.cc
rib_base.hh: $(INTERFACES_DIR)/common.xif $(INTERFACES_DIR)/rib.xif

# RIBCLIENT
libribclientbase_a_SOURCES = ribclient_base.hh ribclient_base.cc
ribclient_base.hh: $(INTERFACES_DIR)/rib_client.xif

# RTRMGR
librtrmgrbase_a_SOURCES = rtrmgr_base.hh rtrmgr_base.cc
rtrmgr_base.hh:	$(INTERFACES_DIR)/common.xif $(INTERFACES_DIR)/rtrmgr.xif

# TEST PEER
libtestpeerbase_a_SOURCES = test_peer_base.hh test_peer_base.cc
test_peer_base.hh: $(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_peer.xif

# XORPSH
libxorpshbase_a_SOURCES = xorpsh_base.hh xorpsh_base.cc
xorpsh_base.hh:	$(INTERFACES_DIR)/common.xif				\
	$(INTERFACES_DIR)/rtrmgr_client.xif

# BGP4MIB
libbgp4mibbase_a_SOURCES = bgp4_mib_base.hh bgp4_mib_base.cc
bgp4_mib_base.hh: $(INTERFACES_DIR)/common.xif

# Add your target's library sources like this:
# Test target
libtestbase_a_SOURCES = test_base.hh test_base.cc
# This is a hack to rebuild files here when interfaces implemented by
# rib target are touched.  It largely duplicates information inside rib.tgt
test_base.hh: $(INTERFACES_DIR)/common.xif $(INTERFACES_DIR)/test.xif


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

@PY_BUILD@%_base.hh %_base.cc %.xrls: %.tgt $(TGTGEN_PY)
@PY_BUILD@	$(PYTHON) $(TGTGEN_PY) -I$(INTERFACES_DIR) $<


