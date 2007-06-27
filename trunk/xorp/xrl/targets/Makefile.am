##
## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/targets/Makefile.am,v 1.66 2007/05/19 01:52:49 pavlin Exp $
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
tgt_files		+= fea.tgt
tgt_files		+= fea_ifmgr_mirror.tgt
tgt_files		+= fib2mrib.tgt
tgt_files		+= finder.tgt
tgt_files		+= finder_client.tgt
tgt_files		+= mfea.tgt
tgt_files		+= mld6igmp.tgt
tgt_files		+= ospfv2.tgt
tgt_files		+= ospfv3.tgt
tgt_files		+= packet_acl.tgt
tgt_files		+= pim.tgt
tgt_files		+= policy.tgt
tgt_files		+= profiler.tgt
tgt_files		+= rib.tgt
tgt_files		+= ribclient.tgt
tgt_files		+= rip.tgt
tgt_files		+= ripng.tgt
tgt_files		+= rtrmgr.tgt
tgt_files		+= show_distances.tgt
tgt_files		+= show_routes.tgt
tgt_files		+= socket_server.tgt
tgt_files		+= static_routes.tgt
tgt_files		+= test.tgt
tgt_files		+= test_fea_ifmgr_mirror.tgt
tgt_files		+= test_finder_events.tgt
tgt_files		+= test_peer.tgt
tgt_files		+= test_socket4.tgt
tgt_files		+= test_socket6.tgt
tgt_files		+= test_xrls.tgt
tgt_files		+= xorp_if_mib.tgt
tgt_files		+= xorpsh.tgt

# Automatically compute the list of the *.xrls files
xrls_files		 = $(tgt_files:%.tgt=%.xrls)

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA		 = $(xrls_files)

# Add your target's library here
noinst_LTLIBRARIES	 = libbgpbase.la
noinst_LTLIBRARIES	+= libbgp4mibbase.la
noinst_LTLIBRARIES	+= libclibase.la
noinst_LTLIBRARIES	+= libcoordbase.la
noinst_LTLIBRARIES	+= libfeabase.la
noinst_LTLIBRARIES	+= libfeaifmgrmirrorbase.la
noinst_LTLIBRARIES	+= libfib2mribbase.la
noinst_LTLIBRARIES	+= libfinderbase.la
noinst_LTLIBRARIES	+= libfinderclientbase.la
noinst_LTLIBRARIES	+= libmfeabase.la
noinst_LTLIBRARIES	+= libmld6igmpbase.la
noinst_LTLIBRARIES	+= libospfv2base.la
noinst_LTLIBRARIES	+= libospfv3base.la
noinst_LTLIBRARIES	+= libpacketaclbase.la
noinst_LTLIBRARIES	+= libpimbase.la
noinst_LTLIBRARIES	+= libpolicybase.la
noinst_LTLIBRARIES	+= libprofileclientbase.la
noinst_LTLIBRARIES	+= libribbase.la
noinst_LTLIBRARIES	+= libribclientbase.la
noinst_LTLIBRARIES	+= libripbase.la
noinst_LTLIBRARIES	+= libripngbase.la
noinst_LTLIBRARIES	+= librtrmgrbase.la
noinst_LTLIBRARIES	+= libshowdistancesbase.la
noinst_LTLIBRARIES	+= libshowroutesbase.la
noinst_LTLIBRARIES	+= libsocketserverbase.la
noinst_LTLIBRARIES	+= libstaticroutesbase.la
noinst_LTLIBRARIES	+= libtestbase.la
noinst_LTLIBRARIES	+= libtestfeaifmgrmirrorbase.la
noinst_LTLIBRARIES	+= libtestfindereventsbase.la
noinst_LTLIBRARIES	+= libtestpeerbase.la
noinst_LTLIBRARIES	+= libtestsocket4.la
noinst_LTLIBRARIES	+= libtestsocket6.la
noinst_LTLIBRARIES	+= libtestxrlsbase.la
noinst_LTLIBRARIES	+= libxorpifmibbase.la
noinst_LTLIBRARIES	+= libxorpshbase.la

# BGP
libbgpbase_la_SOURCES = bgp_base.hh bgp_base.cc
$(srcdir)/bgp_base.hh $(srcdir)/bgp_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/bgp.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/policy_redist6.xif			\
		$(INTERFACES_DIR)/profile.xif				\
		$(INTERFACES_DIR)/rib_client.xif

# BGP4MIB
libbgp4mibbase_la_SOURCES = bgp4_mib_base.hh bgp4_mib_base.cc
$(srcdir)/bgp4_mib_base.hh $(srcdir)/bgp4_mib_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/bgp_mib_traps.xif

# CLI
libclibase_la_SOURCES = cli_base.hh cli_base.cc
$(srcdir)/cli_base.hh $(srcdir)/cli_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/cli_manager.xif

# COORD
libcoordbase_la_SOURCES = coord_base.hh coord_base.cc
$(srcdir)/coord_base.hh $(srcdir)/coord_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/coord.xif				\
		$(INTERFACES_DIR)/datain.xif

# FEA
libfeabase_la_SOURCES = fea_base.hh fea_base.cc
$(srcdir)/fea_base.hh $(srcdir)/fea_base.cc: 				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_click.xif				\
		$(INTERFACES_DIR)/fea_fib.xif				\
		$(INTERFACES_DIR)/fea_ifmgr.xif				\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif		\
		$(INTERFACES_DIR)/fti.xif				\
		$(INTERFACES_DIR)/redist_transaction4.xif		\
		$(INTERFACES_DIR)/redist_transaction6.xif		\
		$(INTERFACES_DIR)/fea_rawlink.xif			\
		$(INTERFACES_DIR)/fea_rawpkt4.xif			\
		$(INTERFACES_DIR)/fea_rawpkt6.xif			\
		$(INTERFACES_DIR)/socket4_locator.xif			\
		$(INTERFACES_DIR)/socket6_locator.xif			\
		$(INTERFACES_DIR)/profile.xif

# FEA IfMgr Mirror
libfeaifmgrmirrorbase_la_SOURCES = fea_ifmgr_mirror_base.hh		\
				  fea_ifmgr_mirror_base.cc
$(srcdir)/fea_ifmgr_mirror_base.hh $(srcdir)/fea_ifmgr_mirror_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_ifmgr_mirror.xif

# Fib2mrib
libfib2mribbase_la_SOURCES = fib2mrib_base.hh fib2mrib_base.cc
$(srcdir)/fib2mrib_base.hh $(srcdir)/fib2mrib_base.cc:			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/fea_fib_client.xif 			\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/fib2mrib.xif

#
# FINDER
#
# If you edit the dependencies here, please up ${XORPSRC}/libxipc/Makefile.am
# too.
#
libfinderbase_la_SOURCES = finder_base.hh finder_base.cc
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
libfinderclientbase_la_SOURCES = finder_client_base.hh finder_client_base.cc
$(srcdir)/finder_client_base.hh $(srcdir)/finder_client_base.cc: 	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_client.xif

# MFEA
libmfeabase_la_SOURCES = mfea_base.hh mfea_base.cc
$(srcdir)/mfea_base.hh $(srcdir)/mfea_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif			\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif		\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/mfea.xif

# MLD6IGMP
libmld6igmpbase_la_SOURCES = mld6igmp_base.hh mld6igmp_base.cc
$(srcdir)/mld6igmp_base.hh $(srcdir)/mld6igmp_base.cc: 			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif 			\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/fea_rawpkt6_client.xif		\
		$(INTERFACES_DIR)/mld6igmp.xif

# OSPF V2
libospfv2base_la_SOURCES = ospfv2_base.hh ospfv2_base.cc
$(srcdir)/ospfv2_base.hh $(srcdir)/ospfv2_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/ospfv2.xif

# OSPF V3
libospfv3base_la_SOURCES = ospfv3_base.hh ospfv3_base.cc
$(srcdir)/ospfv3_base.hh $(srcdir)/ospfv3_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/fea_rawpkt6_client.xif		\
		$(INTERFACES_DIR)/ospfv3.xif

# FIREWALL (FEA)
libpacketaclbase_la_SOURCES = packet_acl_base.hh packet_acl_base.cc
$(srcdir)/packet_acl_base.hh $(srcdir)/packet_acl_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/packet_acl.xif

# PIM
libpimbase_la_SOURCES = pim_base.hh pim_base.cc
$(srcdir)/pim_base.hh $(srcdir)/pim_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif			\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/fea_rawpkt6_client.xif		\
		$(INTERFACES_DIR)/mfea_client.xif			\
		$(INTERFACES_DIR)/redist_transaction4.xif		\
		$(INTERFACES_DIR)/redist_transaction6.xif		\
		$(INTERFACES_DIR)/mld6igmp_client.xif			\
		$(INTERFACES_DIR)/pim.xif

# Policy manager
libpolicybase_la_SOURCES = policy_base.hh policy_base.cc
$(srcdir)/policy_base.hh $(srcdir)/policy_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy.xif

# Profiler
libprofileclientbase_la_SOURCES = profiler_base.hh profiler_base.cc
$(srcdir)/profiler_base.hh $(srcdir)/profiler_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/profile_client.xif

# RIB
libribbase_la_SOURCES = rib_base.hh rib_base.cc
$(srcdir)/rib_base.hh $(srcdir)/rib_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/rib.xif				\
		$(INTERFACES_DIR)/profile.xif

# RIB client
libribclientbase_la_SOURCES = ribclient_base.hh ribclient_base.cc
$(srcdir)/ribclient_base.hh $(srcdir)/ribclient_base.cc: 		\
		$(INTERFACES_DIR)/rib_client.xif

# RIP
libripbase_la_SOURCES = rip_base.hh rip_base.cc
$(srcdir)/rip_base.hh $(srcdir)/rip_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/rip.xif				\
		$(INTERFACES_DIR)/redist4.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# RIPng
libripngbase_la_SOURCES = ripng_base.hh ripng_base.cc
$(srcdir)/ripng_base.hh $(srcdir)/ripng_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist6.xif			\
		$(INTERFACES_DIR)/ripng.xif				\
		$(INTERFACES_DIR)/redist6.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

# RTRMGR
librtrmgrbase_la_SOURCES = rtrmgr_base.hh rtrmgr_base.cc
$(srcdir)/rtrmgr_base.hh $(srcdir)/rtrmgr_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr.xif

libshowdistancesbase_la_SOURCES = show_distances_base.hh show_distances_base.cc
$(srcdir)/show_distances_base.hh $(srcdir)/show_distances_base.cc:	\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif	

libshowroutesbase_la_SOURCES = show_routes_base.hh show_routes_base.cc
$(srcdir)/show_routes_base.hh $(srcdir)/show_routes_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/redist4.xif				\
		$(INTERFACES_DIR)/redist6.xif

# SOCKET SERVER
libsocketserverbase_la_SOURCES  = socket_server_base.hh			\
libsocketserverbase_la_SOURCES += socket_server_base.cc
$(srcdir)/socket_server_base.hh $(srcdir)/socket_server_base.cc:	\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/socket4.xif				\
		$(INTERFACES_DIR)/socket6.xif

# StaticRoutes
libstaticroutesbase_la_SOURCES = static_routes_base.hh static_routes_base.cc
$(srcdir)/static_routes_base.hh $(srcdir)/static_routes_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/static_routes.xif

# TEST FEA IFMGR MIRROR
libtestfeaifmgrmirrorbase_la_SOURCES  = test_fea_ifmgr_mirror_base.hh
libtestfeaifmgrmirrorbase_la_SOURCES += test_fea_ifmgr_mirror_base.cc
$(srcdir)/test_fea_ifmgr_mirror_base.hh					\
$(srcdir)/test_fea_ifmgr_mirror_base.cc:				\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif

#
# TEST FINDER EVENTS
#
# If you edit the dependencies here, please up ${XORPSRC}/libxipc/Makefile.am
# too.
#
libtestfindereventsbase_la_SOURCES  = test_finder_events_base.hh
libtestfindereventsbase_la_SOURCES += test_finder_events_base.cc
$(srcdir)/test_finder_events_base.hh $(srcdir)/test_finder_events_base.hh: \
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif

# TEST PEER
libtestpeerbase_la_SOURCES = test_peer_base.hh test_peer_base.cc
$(srcdir)/test_peer_base.hh $(srcdir)/test_peer_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_peer.xif

# TEST SOCKET4
libtestsocket4_la_SOURCES = test_socket4_base.hh test_socket4_base.cc
$(srcdir)/test_socket4_base.hh $(srcdir)/test_socket4_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# TEST SOCKET6
libtestsocket6_la_SOURCES = test_socket6_base.hh test_socket6_base.cc
$(srcdir)/test_socket6_base.hh $(srcdir)/test_socket6_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

#
# TEST XRLS
#
libtestxrlsbase_la_SOURCES = test_xrls_base.hh test_xrls_base.cc
$(srcdir)/test_xrls_base.hh $(srcdir)/test_xrls_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_xrls.xif

# XORP IF MIB
libxorpifmibbase_la_SOURCES = xorp_if_mib_base.hh xorp_if_mib_base.cc
$(srcdir)/xorp_if_mib_base.hh $(srcdir)/xorp_if_mib_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/xorp_if_mib.xif

# XORPSH
libxorpshbase_la_SOURCES = xorpsh_base.hh xorpsh_base.cc
$(srcdir)/xorpsh_base.hh $(srcdir)/xorpsh_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr_client.xif

# Add your target's library sources like this:
# Test target
libtestbase_la_SOURCES = test_base.hh test_base.cc
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

@PYTHON_BUILD@$(srcdir)/%_base.hh $(srcdir)/%_base.cc %_base.hh %_base.cc    \
@PYTHON_BUILD@$(srcdir)/%.xrls: $(srcdir)/%.tgt $(TGTGEN_PY)
@PYTHON_BUILD@	$(PYTHON) $(TGTGEN_PY) -I$(INTERFACES_DIR) $<


