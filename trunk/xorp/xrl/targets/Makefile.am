##
## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/targets/Makefile.am,v 1.26 2004/02/26 22:46:49 hodson Exp $
##

INTERFACES_DIR=$(top_srcdir)/xrl/interfaces
TARGET_DIR=$(top_srcdir)/xrl/targets

###############################################################################
# Xrl Target related
###############################################################################

# Add your target file here
tgt_files		 = bgp.tgt
tgt_files		+= bgp4_mib.tgt
tgt_files		+= cli.tgt
tgt_files		+= coord.tgt
tgt_files		+= demo_fea_ifmgr_client.tgt
tgt_files		+= fea.tgt
tgt_files		+= fea_ifmgr_mirror.tgt
tgt_files		+= fib2mrib.tgt
tgt_files		+= finder.tgt
tgt_files		+= finder_client.tgt
tgt_files		+= mfea.tgt
tgt_files		+= mld6igmp.tgt
tgt_files		+= ospf.tgt
tgt_files		+= pim.tgt
tgt_files		+= rip.tgt
tgt_files		+= ripng.tgt
tgt_files		+= rib.tgt
tgt_files		+= ribclient.tgt
tgt_files		+= rtrmgr.tgt
tgt_files		+= socket_server.tgt
tgt_files		+= static_routes.tgt
tgt_files		+= test.tgt
tgt_files		+= test_fea_ifmgr_mirror.tgt
tgt_files		+= test_finder_events.tgt
tgt_files		+= test_peer.tgt
tgt_files		+= test_socket4.tgt
tgt_files		+= test_socket6.tgt
tgt_files		+= xorp_if_mib.tgt
tgt_files		+= xorpsh.tgt

# Automatically compute the list of the *.xrls files
xrls_files		 = $(tgt_files:%.tgt=%.xrls)

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA		 = $(xrls_files)

# Add your target's library here
noinst_LIBRARIES	 = libbgpbase.a
noinst_LIBRARIES	+= libbgp4mibbase.a
noinst_LIBRARIES	+= libclibase.a
noinst_LIBRARIES	+= libcoordbase.a
noinst_LIBRARIES	+= libdemofeaifmgrclientbase.a
noinst_LIBRARIES	+= libfeabase.a
noinst_LIBRARIES	+= libfeaifmgrmirrorbase.a
noinst_LIBRARIES	+= libfib2mribbase.a
noinst_LIBRARIES	+= libfinderbase.a
noinst_LIBRARIES	+= libfinderclientbase.a
noinst_LIBRARIES	+= libmfeabase.a
noinst_LIBRARIES	+= libmld6igmpbase.a
noinst_LIBRARIES	+= libospfbase.a
noinst_LIBRARIES	+= libpimbase.a
noinst_LIBRARIES	+= libribbase.a
noinst_LIBRARIES	+= libribclientbase.a
noinst_LIBRARIES	+= libripbase.a
noinst_LIBRARIES	+= libripngbase.a
noinst_LIBRARIES	+= librtrmgrbase.a
noinst_LIBRARIES	+= libsocketserverbase.a
noinst_LIBRARIES	+= libstaticroutesbase.a
noinst_LIBRARIES	+= libtestbase.a
noinst_LIBRARIES	+= libtestfeaifmgrmirrorbase.a
noinst_LIBRARIES	+= libtestfindereventsbase.a
noinst_LIBRARIES	+= libtestpeerbase.a
noinst_LIBRARIES	+= libtestsocket4.a
noinst_LIBRARIES	+= libtestsocket6.a
noinst_LIBRARIES	+= libxorpifmibbase.a
noinst_LIBRARIES	+= libxorpshbase.a

# BGP
libbgpbase_a_SOURCES = bgp_base.hh bgp_base.cc
$(srcdir)/bgp_base.hh $(srcdir)/bgp_base.cc:				\
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
		$(INTERFACES_DIR)/fea_fib.xif				\
		$(INTERFACES_DIR)/fea_ifmgr.xif				\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif		\
		$(INTERFACES_DIR)/fti.xif				\
		$(INTERFACES_DIR)/fea_rawpkt.xif			\
		$(INTERFACES_DIR)/socket4_locator.xif			\
		$(INTERFACES_DIR)/socket6_locator.xif

# FEA IfMgr Mirror
libfeaifmgrmirrorbase_a_SOURCES = fea_ifmgr_mirror_base.hh		\
				  fea_ifmgr_mirror_base.cc
$(srcdir)/fea_ifmgr_mirror_base.hh $(srcdir)/fea_ifmgr_mirror_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_ifmgr_mirror.xif

# Fib2mrib
libfib2mribbase_a_SOURCES = fib2mrib_base.hh fib2mrib_base.cc
$(srcdir)/fib2mrib_base.hh $(srcdir)/fib2mrib_base.cc:			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fib2mrib.xif

#
# FINDER
#
# If you edit the dependencies here, please up ${XORPSRC}/libxipc/Makefile.am
# too.
#
libfinderbase_a_SOURCES = finder_base.hh finder_base.cc
$(srcdir)/finder_base.hh $(srcdir)/finder_base.cc : 			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder.xif				\
		$(INTERFACES_DIR)/finder_event_notifier.xif

#
# FINDER CLIENT
#
# If you edit the dependencies here, please up ${XORPSRC}/libxipc/Makefile.am
# too.
#
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

# RIP
libripbase_a_SOURCES = rip_base.hh rip_base.cc
$(srcdir)/rip_base.hh $(srcdir)/rip_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/rip.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# RIP NG
libripngbase_a_SOURCES = ripng_base.hh ripng_base.cc
$(srcdir)/ripng_base.hh $(srcdir)/ripng_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/ripng.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

# RTRMGR
librtrmgrbase_a_SOURCES = rtrmgr_base.hh rtrmgr_base.cc
$(srcdir)/rtrmgr_base.hh $(srcdir)/rtrmgr_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr.xif

# SOCKET SERVER
libsocketserverbase_a_SOURCES  = socket_server_base.hh			\
libsocketserverbase_a_SOURCES += socket_server_base.cc
$(srcdir)/socket_server_base.hh $(srcdir)/socket_server_base.cc:	\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/socket4.xif				\
		$(INTERFACES_DIR)/socket6.xif

# StaticRoutes
libstaticroutesbase_a_SOURCES = static_routes_base.hh static_routes_base.cc
$(srcdir)/static_routes_base.hh $(srcdir)/static_routes_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/static_routes.xif

# TEST FEA IFMGR MIRROR
libtestfeaifmgrmirrorbase_a_SOURCES  = test_fea_ifmgr_mirror_base.hh
libtestfeaifmgrmirrorbase_a_SOURCES += test_fea_ifmgr_mirror_base.cc
$(srcdir)/test_fea_ifmgr_mirror_base.hh					\
$(srcdir)/test_fea_ifmgr_mirror_base.cc:				\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif

#
# TEST FINDER EVENTS
#
# If you edit the dependencies here, please up ${XORPSRC}/libxipc/Makefile.am
# too.
#
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

# TEST SOCKET4
libtestsocket4_a_SOURCES = test_socket4_base.hh test_socket4_base.cc
$(srcdir)/test_socket4_base.hh $(srcdir)/test_socket4_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# TEST SOCKET6
libtestsocket6_a_SOURCES = test_socket6_base.hh test_socket6_base.cc
$(srcdir)/test_socket6_base.hh $(srcdir)/test_socket6_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

# XORP IF MIB
libxorpifmibbase_a_SOURCES = xorp_if_mib_base.hh xorp_if_mib_base.cc
$(srcdir)/xorp_if_mib_base.hh $(srcdir)/xorp_if_mib_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/xorp_if_mib.xif

# XORPSH
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

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = $(tgt_files)
EXTRA_DIST	+= $(xrls_files)

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


