## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/xrl/interfaces/Makefile.am,v 1.41 2009/01/22 17:53:08 jtc Exp $
##

###############################################################################
# Client Interface related
###############################################################################

# BGP MIB traps
lib_LTLIBRARIES = libbgpmibtrapsxif.la
libbgpmibtrapsxif_la_SOURCES = bgp_mib_traps_xif.hh bgp_mib_traps_xif.cc
libbgpmibtrapsxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libbgpmibtrapsxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libbgpmibtrapsxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# BGP Client Interface
lib_LTLIBRARIES += libbgpxif.la
libbgpxif_la_SOURCES = bgp_xif.hh bgp_xif.cc
libbgpxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libbgpxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libbgpxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Command-Line Interface Manager Interface
lib_LTLIBRARIES += libclimanagerxif.la
libclimanagerxif_la_SOURCES = cli_manager_xif.hh cli_manager_xif.cc
libclimanagerxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libclimanagerxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libclimanagerxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Command-Line Interface Processor Interface
lib_LTLIBRARIES += libcliprocessorxif.la
libcliprocessorxif_la_SOURCES = cli_processor_xif.hh cli_processor_xif.cc
libcliprocessorxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libcliprocessorxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libcliprocessorxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Common Client Interface
lib_LTLIBRARIES  += libcommonxif.la
libcommonxif_la_SOURCES = common_xif.hh common_xif.cc
libcommonxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libcommonxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libcommonxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Interface to the coordinating BGP test process
lib_LTLIBRARIES += libcoordxif.la
libcoordxif_la_SOURCES = coord_xif.hh coord_xif.cc
libcoordxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libcoordxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libcoordxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Datain client of the test peer interface.
lib_LTLIBRARIES += libdatainxif.la
libdatainxif_la_SOURCES = datain_xif.hh datain_xif.cc
libdatainxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libdatainxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libdatainxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Click Interface
lib_LTLIBRARIES += libfeaclickxif.la
libfeaclickxif_la_SOURCES = fea_click_xif.hh fea_click_xif.cc
libfeaclickxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeaclickxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaclickxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Client Interface
lib_LTLIBRARIES += libfeaifmgrxif.la
libfeaifmgrxif_la_SOURCES = fea_ifmgr_xif.hh fea_ifmgr_xif.cc
libfeaifmgrxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeaifmgrxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaifmgrxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA FIB Interface
lib_LTLIBRARIES += libfeafibxif.la
libfeafibxif_la_SOURCES = fea_fib_xif.hh fea_fib_xif.cc
libfeafibxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeafibxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeafibxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA FIB Client Interface
lib_LTLIBRARIES += libfeafibclientxif.la
libfeafibclientxif_la_SOURCES = fea_fib_client_xif.hh fea_fib_client_xif.cc
libfeafibclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeafibclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeafibclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Firewall Interface
lib_LTLIBRARIES += libfeafirewallxif.la
libfeafirewallxif_la_SOURCES = fea_firewall_xif.hh fea_firewall_xif.cc
libfeafirewallxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeafirewallxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeafirewallxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Interface Manager Mirror Interface
lib_LTLIBRARIES += libfeaifmgrmirrorxif.la
libfeaifmgrmirrorxif_la_SOURCES  = fea_ifmgr_mirror_xif.hh
libfeaifmgrmirrorxif_la_SOURCES += fea_ifmgr_mirror_xif.cc
libfeaifmgrmirrorxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeaifmgrmirrorxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaifmgrmirrorxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Interface Manager Replicator Interface
lib_LTLIBRARIES += libfeaifmgrreplicatorxif.la
libfeaifmgrreplicatorxif_la_SOURCES  = fea_ifmgr_replicator_xif.hh
libfeaifmgrreplicatorxif_la_SOURCES += fea_ifmgr_replicator_xif.cc
libfeaifmgrreplicatorxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfeaifmgrreplicatorxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfeaifmgrreplicatorxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Fib2mrib Interface
lib_LTLIBRARIES += libfib2mribxif.la
libfib2mribxif_la_SOURCES = fib2mrib_xif.hh fib2mrib_xif.cc
libfib2mribxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfib2mribxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfib2mribxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Forwarding Table Interface Client Interface
lib_LTLIBRARIES += libftixif.la
libftixif_la_SOURCES = fti_xif.hh fti_xif.cc
libftixif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libftixif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libftixif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Raw Link-Level I/O Server Interface
lib_LTLIBRARIES += libfearawlinkxif.la
libfearawlinkxif_la_SOURCES  = fea_rawlink_xif.hh
libfearawlinkxif_la_SOURCES += fea_rawlink_xif.cc
libfearawlinkxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawlinkxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawlinkxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA Raw Link-Level I/O Client Interface
lib_LTLIBRARIES += libfearawlinkclientxif.la
libfearawlinkclientxif_la_SOURCES  = fea_rawlink_client_xif.hh
libfearawlinkclientxif_la_SOURCES += fea_rawlink_client_xif.cc
libfearawlinkclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawlinkclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawlinkclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA IPv4 Raw Packet Server Interface
lib_LTLIBRARIES += libfearawpkt4xif.la
libfearawpkt4xif_la_SOURCES  = fea_rawpkt4_xif.hh
libfearawpkt4xif_la_SOURCES += fea_rawpkt4_xif.cc
libfearawpkt4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawpkt4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawpkt4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA IPv4 Raw Packet Client Interface
lib_LTLIBRARIES += libfearawpkt4clientxif.la
libfearawpkt4clientxif_la_SOURCES  = fea_rawpkt4_client_xif.hh
libfearawpkt4clientxif_la_SOURCES += fea_rawpkt4_client_xif.cc
libfearawpkt4clientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawpkt4clientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawpkt4clientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA IPv6 Raw Packet Server Interface
lib_LTLIBRARIES += libfearawpkt6xif.la
libfearawpkt6xif_la_SOURCES  = fea_rawpkt6_xif.hh
libfearawpkt6xif_la_SOURCES += fea_rawpkt6_xif.cc
libfearawpkt6xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawpkt6xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawpkt6xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# FEA IPv6 Raw Packet Client Interface
lib_LTLIBRARIES += libfearawpkt6clientxif.la
libfearawpkt6clientxif_la_SOURCES  = fea_rawpkt6_client_xif.hh
libfearawpkt6clientxif_la_SOURCES += fea_rawpkt6_client_xif.cc
libfearawpkt6clientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfearawpkt6clientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfearawpkt6clientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Finder Interface
lib_LTLIBRARIES += libfinderxif.la
libfinderxif_la_SOURCES = finder_xif.hh finder_xif.cc
libfinderxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfinderxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfinderxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Finder Event Notifier Interface
lib_LTLIBRARIES += libfindereventnotifierxif.la
libfindereventnotifierxif_la_SOURCES  = finder_event_notifier_xif.hh
libfindereventnotifierxif_la_SOURCES += finder_event_notifier_xif.cc
libfindereventnotifierxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfindereventnotifierxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfindereventnotifierxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Finder Event Observer Interface
lib_LTLIBRARIES += libfindereventobserverxif.la
libfindereventobserverxif_la_SOURCES  = finder_event_observer_xif.hh
libfindereventobserverxif_la_SOURCES += finder_event_observer_xif.cc
libfindereventobserverxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfindereventobserverxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfindereventobserverxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Finder Client Interface (client of Finder)
lib_LTLIBRARIES += libfinderclientxif.la
libfinderclientxif_la_SOURCES = finder_client_xif.hh finder_client_xif.cc
libfinderclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libfinderclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libfinderclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Multicast FEA Interface
lib_LTLIBRARIES += libmfeaxif.la
libmfeaxif_la_SOURCES = mfea_xif.hh mfea_xif.cc
libmfeaxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmfeaxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmfeaxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Multicast FEA Client Interface
lib_LTLIBRARIES += libmfeaclientxif.la
libmfeaclientxif_la_SOURCES = mfea_client_xif.hh mfea_client_xif.cc
libmfeaclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmfeaclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmfeaclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# MLD6/IGMP Interface
lib_LTLIBRARIES += libmld6igmpxif.la
libmld6igmpxif_la_SOURCES = mld6igmp_xif.hh mld6igmp_xif.cc
libmld6igmpxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmld6igmpxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmld6igmpxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# MLD6/IGMP Client Interface
lib_LTLIBRARIES += libmld6igmpclientxif.la
libmld6igmpclientxif_la_SOURCES = mld6igmp_client_xif.hh mld6igmp_client_xif.cc
libmld6igmpclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libmld6igmpclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libmld6igmpclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# OLSR V1 IPv4 Interface
lib_LTLIBRARIES += libolsr4xif.la
libolsr4xif_la_SOURCES = olsr4_xif.hh olsr4_xif.cc
libolsr4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libolsr4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libolsr4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# OSPF V2 Interface
lib_LTLIBRARIES += libospfv2xif.la
libospfv2xif_la_SOURCES = ospfv2_xif.hh ospfv2_xif.cc
libospfv2xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libospfv2xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libospfv2xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# OSPF V3 Interface
lib_LTLIBRARIES += libospfv3xif.la
libospfv3xif_la_SOURCES = ospfv3_xif.hh ospfv3_xif.cc
libospfv3xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libospfv3xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libospfv3xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# PIM Interface
lib_LTLIBRARIES += libpimxif.la
libpimxif_la_SOURCES = pim_xif.hh pim_xif.cc
libpimxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpimxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpimxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Policy manager Interface
lib_LTLIBRARIES += libpolicyxif.la
libpolicyxif_la_SOURCES = policy_xif.hh policy_xif.cc
libpolicyxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpolicyxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpolicyxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Policy backend Interface
lib_LTLIBRARIES += libpolicybackendxif.la
libpolicybackendxif_la_SOURCES = policy_backend_xif.hh policy_backend_xif.cc
libpolicybackendxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpolicybackendxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpolicybackendxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Policy route redistribution IPv4 Interface
lib_LTLIBRARIES += libpolicyredist4xif.la
libpolicyredist4xif_la_SOURCES = policy_redist4_xif.hh policy_redist4_xif.cc
libpolicyredist4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpolicyredist4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpolicyredist4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Policy route redistribution IPv6 Interface
lib_LTLIBRARIES += libpolicyredist6xif.la
libpolicyredist6xif_la_SOURCES = policy_redist6_xif.hh policy_redist6_xif.cc
libpolicyredist6xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libpolicyredist6xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libpolicyredist6xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Profiling Interface
lib_LTLIBRARIES += libprofilexif.la
libprofilexif_la_SOURCES = profile_xif.hh profile_xif.cc
libprofilexif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libprofilexif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libprofilexif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Profiling Client Interface
lib_LTLIBRARIES += libprofileclientxif.la
libprofileclientxif_la_SOURCES = profile_client_xif.hh profile_client_xif.cc
libprofileclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libprofileclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libprofileclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Redistribution Interface to Routing Protocols
lib_LTLIBRARIES += libredist4xif.la
libredist4xif_la_SOURCES = redist4_xif.hh redist4_xif.cc
libredist4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libredist4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libredist4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

lib_LTLIBRARIES += libredist6xif.la
libredist6xif_la_SOURCES = redist6_xif.hh redist6_xif.cc
libredist6xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libredist6xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libredist6xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Transaction-based Redistribution Interface to Routing Protocols
lib_LTLIBRARIES += libredisttransaction4xif.la
libredisttransaction4xif_la_SOURCES  = redist_transaction4_xif.hh
libredisttransaction4xif_la_SOURCES += redist_transaction4_xif.cc
libredisttransaction4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libredisttransaction4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libredisttransaction4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

lib_LTLIBRARIES += libredisttransaction6xif.la
libredisttransaction6xif_la_SOURCES  = redist_transaction6_xif.hh
libredisttransaction6xif_la_SOURCES += redist_transaction6_xif.cc
libredisttransaction6xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libredisttransaction6xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libredisttransaction6xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RIB Interface to Routing Protocols
lib_LTLIBRARIES += libribxif.la
libribxif_la_SOURCES = rib_xif.hh rib_xif.cc
libribxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libribxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libribxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RIB Client Interface supported by BGP
lib_LTLIBRARIES += libribclientxif.la
libribclientxif_la_SOURCES = rib_client_xif.hh rib_client_xif.cc
libribclientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libribclientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libribclientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RIP Interface
lib_LTLIBRARIES += libripxif.la
libripxif_la_SOURCES = rip_xif.hh rip_xif.cc
libripxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libripxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libripxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RIPng Interface
lib_LTLIBRARIES += libripngxif.la
libripngxif_la_SOURCES = ripng_xif.hh ripng_xif.cc
libripngxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libripngxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libripngxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RTRMGR Interface
lib_LTLIBRARIES += librtrmgrxif.la
librtrmgrxif_la_SOURCES = rtrmgr_xif.hh rtrmgr_xif.cc
librtrmgrxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
librtrmgrxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
librtrmgrxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# RTRMGR Client Interface
lib_LTLIBRARIES += librtrmgr_clientxif.la
librtrmgr_clientxif_la_SOURCES = rtrmgr_client_xif.hh rtrmgr_client_xif.cc
librtrmgr_clientxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
librtrmgr_clientxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
librtrmgr_clientxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# IPv4 Socket Interface
lib_LTLIBRARIES += libsocket4xif.la
libsocket4xif_la_SOURCES = socket4_xif.hh socket4_xif.cc
libsocket4xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libsocket4xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libsocket4xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# IPv4 Socket User Interface
lib_LTLIBRARIES += libsocket4userxif.la
libsocket4userxif_la_SOURCES = socket4_user_xif.hh socket4_user_xif.cc
libsocket4userxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libsocket4userxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libsocket4userxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# IPv6 Socket Interface
lib_LTLIBRARIES += libsocket6xif.la
libsocket6xif_la_SOURCES = socket6_xif.hh socket6_xif.cc
libsocket6xif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libsocket6xif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libsocket6xif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# IPv6 Socket User Interface
lib_LTLIBRARIES += libsocket6userxif.la
libsocket6userxif_la_SOURCES = socket6_user_xif.hh socket6_user_xif.cc
libsocket6userxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libsocket6userxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libsocket6userxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# StaticRoutes Interface
lib_LTLIBRARIES += libstaticroutesxif.la
libstaticroutesxif_la_SOURCES = static_routes_xif.hh static_routes_xif.cc
libstaticroutesxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libstaticroutesxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libstaticroutesxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Test Peer Interface
lib_LTLIBRARIES += libtestpeerxif.la
libtestpeerxif_la_SOURCES = test_peer_xif.hh test_peer_xif.cc
libtestpeerxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestpeerxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestpeerxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# Test XRLs Interface
lib_LTLIBRARIES += libtestxrlsxif.la
libtestxrlsxif_la_SOURCES = test_xrls_xif.hh test_xrls_xif.cc
libtestxrlsxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestxrlsxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestxrlsxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# VRRP Interface
lib_LTLIBRARIES += libvrrpxif.la
libvrrpxif_la_SOURCES = vrrp_xif.hh vrrp_xif.cc
libvrrpxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libvrrpxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libvrrpxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# XORP MIBS Interface
lib_LTLIBRARIES += libxorpifmibxif.la
libxorpifmibxif_la_SOURCES = xorp_if_mib_xif.hh xorp_if_mib_xif.cc
libxorpifmibxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libxorpifmibxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libxorpifmibxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

#
# Sample Test Client Interface
#
lib_LTLIBRARIES += libtestxif.la
libtestxif_la_SOURCES = test_xif.hh test_xif.cc
libtestxif_la_LIBADD  = $(top_builddir)/libxipc/libxipc.la
libtestxif_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libtestxif_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

###############################################################################
# Static Pattern Rules
###############################################################################

SCRIPT_DIR=$(top_srcdir)/xrl/scripts
CLNTGEN_PY=$(SCRIPT_DIR)/clnt-gen

@PYTHON_BUILD@$(srcdir)/%_xif.hh $(srcdir)/%_xif.cc: $(srcdir)/%.xif $(CLNTGEN_PY)
@PYTHON_BUILD@	$(PYTHON) $(CLNTGEN_PY) $<
