##
## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/targets/Makefile.am,v 1.74 2009/01/22 17:53:11 jtc Exp $
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
tgt_files		+= olsr4.tgt
tgt_files		+= ospfv2.tgt
tgt_files		+= ospfv3.tgt
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
tgt_files		+= static_routes.tgt
tgt_files		+= test.tgt
tgt_files		+= test_fea_ifmgr_mirror.tgt
tgt_files		+= test_fea_rawlink.tgt
tgt_files		+= test_finder_events.tgt
tgt_files		+= test_peer.tgt
tgt_files		+= test_socket4.tgt
tgt_files		+= test_socket6.tgt
tgt_files		+= test_xrls.tgt
tgt_files		+= vrrp.tgt
tgt_files		+= xorp_if_mib.tgt
tgt_files		+= xorpsh.tgt

# Automatically compute the list of the *.xrls files
xrls_files		 = $(tgt_files:%.tgt=%.xrls)

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA		 = $(xrls_files)

# Add your target's library here
lib_LTLIBRARIES	 = libbgpbase.la
lib_LTLIBRARIES	+= libbgp4mibbase.la
lib_LTLIBRARIES	+= libclibase.la
lib_LTLIBRARIES	+= libcoordbase.la
lib_LTLIBRARIES	+= libfeabase.la
lib_LTLIBRARIES	+= libfeaifmgrmirrorbase.la
lib_LTLIBRARIES	+= libfib2mribbase.la
lib_LTLIBRARIES	+= libfinderbase.la
lib_LTLIBRARIES	+= libfinderclientbase.la
lib_LTLIBRARIES	+= libmfeabase.la
lib_LTLIBRARIES	+= libmld6igmpbase.la
lib_LTLIBRARIES	+= libolsr4base.la
lib_LTLIBRARIES	+= libospfv2base.la
lib_LTLIBRARIES	+= libospfv3base.la
lib_LTLIBRARIES	+= libpimbase.la
lib_LTLIBRARIES	+= libpolicybase.la
lib_LTLIBRARIES	+= libprofileclientbase.la
lib_LTLIBRARIES	+= libribbase.la
lib_LTLIBRARIES	+= libribclientbase.la
lib_LTLIBRARIES	+= libripbase.la
lib_LTLIBRARIES	+= libripngbase.la
lib_LTLIBRARIES	+= librtrmgrbase.la
lib_LTLIBRARIES	+= libshowdistancesbase.la
lib_LTLIBRARIES	+= libshowroutesbase.la
lib_LTLIBRARIES	+= libstaticroutesbase.la
lib_LTLIBRARIES	+= libtestbase.la
lib_LTLIBRARIES	+= libtestfeaifmgrmirrorbase.la
lib_LTLIBRARIES	+= libtestfindereventsbase.la
lib_LTLIBRARIES	+= libtestpeerbase.la
lib_LTLIBRARIES	+= libtestfearawlink.la
lib_LTLIBRARIES	+= libtestsocket4.la
lib_LTLIBRARIES	+= libtestsocket6.la
lib_LTLIBRARIES	+= libtestxrlsbase.la
lib_LTLIBRARIES	+= libvrrpbase.la
lib_LTLIBRARIES	+= libxorpifmibbase.la
lib_LTLIBRARIES	+= libxorpshbase.la

# BGP
libbgpbase_la_SOURCES = bgp_base.hh bgp_base.cc
libbgpbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libbgpbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libbgpbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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
libbgp4mibbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libbgp4mibbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libbgp4mibbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/bgp4_mib_base.hh $(srcdir)/bgp4_mib_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/bgp_mib_traps.xif

# CLI
libclibase_la_SOURCES = cli_base.hh cli_base.cc
libclibase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libclibase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libclibase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/cli_base.hh $(srcdir)/cli_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/cli_manager.xif

# COORD
libcoordbase_la_SOURCES = coord_base.hh coord_base.cc
libcoordbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libcoordbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libcoordbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/coord_base.hh $(srcdir)/coord_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/coord.xif				\
		$(INTERFACES_DIR)/datain.xif

# FEA
libfeabase_la_SOURCES = fea_base.hh fea_base.cc
libfeabase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeabase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeabase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/fea_base.hh $(srcdir)/fea_base.cc: 				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/fea_click.xif				\
		$(INTERFACES_DIR)/fea_fib.xif				\
		$(INTERFACES_DIR)/fea_firewall.xif			\
		$(INTERFACES_DIR)/fea_ifmgr.xif				\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif		\
		$(INTERFACES_DIR)/fti.xif				\
		$(INTERFACES_DIR)/redist_transaction4.xif		\
		$(INTERFACES_DIR)/redist_transaction6.xif		\
		$(INTERFACES_DIR)/fea_rawlink.xif			\
		$(INTERFACES_DIR)/fea_rawpkt4.xif			\
		$(INTERFACES_DIR)/fea_rawpkt6.xif			\
		$(INTERFACES_DIR)/socket4.xif				\
		$(INTERFACES_DIR)/socket6.xif				\
		$(INTERFACES_DIR)/profile.xif

# FEA IfMgr Mirror
libfeaifmgrmirrorbase_la_SOURCES = fea_ifmgr_mirror_base.hh		\
				  fea_ifmgr_mirror_base.cc
libfeaifmgrmirrorbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeaifmgrmirrorbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaifmgrmirrorbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/fea_ifmgr_mirror_base.hh $(srcdir)/fea_ifmgr_mirror_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/fea_ifmgr_mirror.xif

# Fib2mrib
libfib2mribbase_la_SOURCES = fib2mrib_base.hh fib2mrib_base.cc
libfib2mribbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfib2mribbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfib2mribbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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
libfinderbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfinderbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfinderbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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
libfinderclientbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfinderclientbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfinderclientbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/finder_client_base.hh $(srcdir)/finder_client_base.cc: 	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_client.xif

# MFEA
libmfeabase_la_SOURCES = mfea_base.hh mfea_base.cc
libmfeabase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmfeabase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmfeabase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/mfea_base.hh $(srcdir)/mfea_base.cc:				\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif			\
		$(INTERFACES_DIR)/fea_ifmgr_replicator.xif		\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/mfea.xif

# MLD6IGMP
libmld6igmpbase_la_SOURCES = mld6igmp_base.hh mld6igmp_base.cc
libmld6igmpbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmld6igmpbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmld6igmpbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/mld6igmp_base.hh $(srcdir)/mld6igmp_base.cc: 			\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/cli_processor.xif 			\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/fea_rawpkt6_client.xif		\
		$(INTERFACES_DIR)/mld6igmp.xif

# OLSR V1 IPv4
libolsr4base_la_SOURCES = olsr4_base.hh olsr4_base.cc
libolsr4base_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libolsr4base_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libolsr4base_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/olsr4_base.hh $(srcdir)/olsr4_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/profile.xif				\
		$(INTERFACES_DIR)/socket4_user.xif			\
		$(INTERFACES_DIR)/olsr4.xif

# OSPF V2
libospfv2base_la_SOURCES = ospfv2_base.hh ospfv2_base.cc
libospfv2base_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libospfv2base_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libospfv2base_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/ospfv2_base.hh $(srcdir)/ospfv2_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/ospfv2.xif

# OSPF V3
libospfv3base_la_SOURCES = ospfv3_base.hh ospfv3_base.cc
libospfv3base_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libospfv3base_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libospfv3base_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/ospfv3_base.hh $(srcdir)/ospfv3_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/fea_rawpkt4_client.xif		\
		$(INTERFACES_DIR)/fea_rawpkt6_client.xif		\
		$(INTERFACES_DIR)/ospfv3.xif

# PIM
libpimbase_la_SOURCES = pim_base.hh pim_base.cc
libpimbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpimbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpimbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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
libpolicybase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpolicybase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpolicybase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/policy_base.hh $(srcdir)/policy_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy.xif

# Profiler
libprofileclientbase_la_SOURCES = profiler_base.hh profiler_base.cc
libprofileclientbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libprofileclientbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libprofileclientbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/profiler_base.hh $(srcdir)/profiler_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/profile_client.xif

# RIB
libribbase_la_SOURCES = rib_base.hh rib_base.cc
libribbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libribbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libribbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/rib_base.hh $(srcdir)/rib_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/rib.xif				\
		$(INTERFACES_DIR)/profile.xif

# RIB client
libribclientbase_la_SOURCES = ribclient_base.hh ribclient_base.cc
libribclientbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libribclientbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libribclientbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/ribclient_base.hh $(srcdir)/ribclient_base.cc: 		\
		$(INTERFACES_DIR)/rib_client.xif

# RIP
libripbase_la_SOURCES = rip_base.hh rip_base.cc
libripbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libripbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libripbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/rip_base.hh $(srcdir)/rip_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist4.xif			\
		$(INTERFACES_DIR)/rip.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# RIPng
libripngbase_la_SOURCES = ripng_base.hh ripng_base.cc
libripngbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libripngbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libripngbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/ripng_base.hh $(srcdir)/ripng_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/policy_redist6.xif			\
		$(INTERFACES_DIR)/ripng.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

# RTRMGR
librtrmgrbase_la_SOURCES = rtrmgr_base.hh rtrmgr_base.cc
librtrmgrbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
librtrmgrbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
librtrmgrbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/rtrmgr_base.hh $(srcdir)/rtrmgr_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr.xif

libshowdistancesbase_la_SOURCES = show_distances_base.hh show_distances_base.cc
libshowdistancesbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libshowdistancesbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libshowdistancesbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/show_distances_base.hh $(srcdir)/show_distances_base.cc:	\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif	

libshowroutesbase_la_SOURCES = show_routes_base.hh show_routes_base.cc
libshowroutesbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libshowroutesbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libshowroutesbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/show_routes_base.hh $(srcdir)/show_routes_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/redist4.xif				\
		$(INTERFACES_DIR)/redist6.xif

# StaticRoutes
libstaticroutesbase_la_SOURCES = static_routes_base.hh static_routes_base.cc
libstaticroutesbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libstaticroutesbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libstaticroutesbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/static_routes_base.hh $(srcdir)/static_routes_base.cc:	\
		$(INTERFACES_DIR)/common.xif 				\
		$(INTERFACES_DIR)/finder_event_observer.xif		\
		$(INTERFACES_DIR)/policy_backend.xif			\
		$(INTERFACES_DIR)/static_routes.xif

# TEST FEA IFMGR MIRROR
libtestfeaifmgrmirrorbase_la_SOURCES  = test_fea_ifmgr_mirror_base.hh
libtestfeaifmgrmirrorbase_la_SOURCES += test_fea_ifmgr_mirror_base.cc
libtestfeaifmgrmirrorbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestfeaifmgrmirrorbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestfeaifmgrmirrorbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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
libtestfindereventsbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestfindereventsbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestfindereventsbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_finder_events_base.hh $(srcdir)/test_finder_events_base.hh: \
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/finder_event_observer.xif

# TEST PEER
libtestpeerbase_la_SOURCES = test_peer_base.hh test_peer_base.cc
libtestpeerbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestpeerbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestpeerbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_peer_base.hh $(srcdir)/test_peer_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_peer.xif

# TEST RAWLINK
libtestfearawlink_la_SOURCES  = test_fea_rawlink_base.hh
libtestfearawlink_la_SOURCES += test_fea_rawlink_base.cc
libtestfearawlink_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestfearawlink_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestfearawlink_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_fea_rawlink.hh $(srcdir)/test_fea_rawlink.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/fea_rawlink.xif

# TEST SOCKET4
libtestsocket4_la_SOURCES = test_socket4_base.hh test_socket4_base.cc
libtestsocket4_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestsocket4_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestsocket4_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_socket4_base.hh $(srcdir)/test_socket4_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket4_user.xif

# TEST SOCKET6
libtestsocket6_la_SOURCES = test_socket6_base.hh test_socket6_base.cc
libtestsocket6_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestsocket6_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestsocket6_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_socket6_base.hh $(srcdir)/test_socket6_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/socket6_user.xif

#
# TEST XRLS
#
libtestxrlsbase_la_SOURCES = test_xrls_base.hh test_xrls_base.cc
libtestxrlsbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestxrlsbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestxrlsbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/test_xrls_base.hh $(srcdir)/test_xrls_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/test_xrls.xif

# VRRP
libvrrpbase_la_SOURCES = vrrp_base.hh vrrp_base.cc
libvrrpbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libvrrpbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libvrrpbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/vrrp_base.hh $(srcdir)/vrrp_base.cc:				\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/vrrp.xif

# XORP IF MIB
libxorpifmibbase_la_SOURCES = xorp_if_mib_base.hh xorp_if_mib_base.cc
libxorpifmibbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libxorpifmibbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libxorpifmibbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/xorp_if_mib_base.hh $(srcdir)/xorp_if_mib_base.cc:		\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/xorp_if_mib.xif

# XORPSH
libxorpshbase_la_SOURCES = xorpsh_base.hh xorpsh_base.cc
libxorpshbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libxorpshbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libxorpshbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
$(srcdir)/xorpsh_base.hh $(srcdir)/xorpsh_base.cc:			\
		$(INTERFACES_DIR)/common.xif				\
		$(INTERFACES_DIR)/rtrmgr_client.xif

# Add your target's library sources like this:
# Test target
libtestbase_la_SOURCES = test_base.hh test_base.cc
libtestbase_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestbase_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestbase_la_LIBADD += $(top_builddir)/libxorp/libxorp.la
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

@PYTHON_BUILD@$(srcdir)/%_base.hh $(srcdir)/%_base.cc \
@PYTHON_BUILD@$(srcdir)/%.xrls: $(srcdir)/%.tgt $(TGTGEN_PY)
@PYTHON_BUILD@	$(PYTHON) $(TGTGEN_PY) -I$(INTERFACES_DIR) $<


