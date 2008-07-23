// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/ospf/test_routing.cc,v 1.28 2008/01/04 03:16:58 pavlin Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/tlv.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <map>
#include <list>
#include <set>
#include <deque>

#include "libproto/spt.hh"

#include "ospf.hh"
#include "debug_io.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "test_common.hh"


// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

void
p2p(OspfTypes::Version version, RouterLsa *rlsa, OspfTypes::RouterID id,
    uint32_t link_data, uint32_t metric)
{
     RouterLink rl(version);

     rl.set_type(RouterLink::p2p);
     rl.set_metric(metric);

    switch(version) {
    case OspfTypes::V2:
	rl.set_link_id(id);
	rl.set_link_data(link_data);
	break;
    case OspfTypes::V3:
	rl.set_interface_id(id);
	rl.set_neighbour_interface_id(0);
	rl.set_neighbour_router_id(id);
	break;
    }
    rlsa->get_router_links().push_back(rl);
}

void
transit(OspfTypes::Version version, RouterLsa *rlsa, OspfTypes::RouterID id,
	uint32_t link_data, uint32_t metric)
{
     RouterLink rl(version);

     rl.set_type(RouterLink::transit);
     rl.set_metric(metric);

    switch(version) {
    case OspfTypes::V2:
	rl.set_link_id(id);
	rl.set_link_data(link_data);
	break;
    case OspfTypes::V3:
	rl.set_interface_id(id);
	rl.set_neighbour_interface_id(0);
	rl.set_neighbour_router_id(id);
	break;
    }
    rlsa->get_router_links().push_back(rl);
}

void
stub(OspfTypes::Version version, RouterLsa *rlsa, OspfTypes::RouterID id,
     uint32_t link_data, uint32_t metric)
{
     RouterLink rl(version);

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	return;
	break;
    }

     rl.set_type(RouterLink::stub);
     rl.set_metric(metric);

     switch(version) {
     case OspfTypes::V2:
	 rl.set_link_id(id);
	 rl.set_link_data(link_data);
	break;
     case OspfTypes::V3:
	 rl.set_interface_id(id);
	 rl.set_neighbour_interface_id(0);
	 rl.set_neighbour_router_id(id);
	 break;
     }
     rlsa->get_router_links().push_back(rl);
}

Lsa::LsaRef
create_router_lsa(OspfTypes::Version version, uint32_t link_state_id,
		  uint32_t advertising_router)
{
     RouterLsa *rlsa = new RouterLsa(version);
     Lsa_header& header = rlsa->get_header();

     uint32_t options = compute_options(version, OspfTypes::NORMAL);

     // Set the header fields.
     switch(version) {
     case OspfTypes::V2:
	 header.set_options(options);
	 break;
     case OspfTypes::V3:
	 rlsa->set_options(options);
	 break;
     }

     header.set_link_state_id(link_state_id);
     header.set_advertising_router(advertising_router);

     return Lsa::LsaRef(rlsa);
}

Lsa::LsaRef
create_network_lsa(OspfTypes::Version version, uint32_t link_state_id,
		   uint32_t advertising_router, uint32_t mask)
{
    NetworkLsa *nlsa = new NetworkLsa(version);
    Lsa_header& header = nlsa->get_header();

    uint32_t options = compute_options(version, OspfTypes::NORMAL);

    // Set the header fields.
    switch(version) {
    case OspfTypes::V2:
	header.set_options(options);
	nlsa->set_network_mask(mask);
	break;
    case OspfTypes::V3:
	nlsa->set_options(options);
	break;
    }

    header.set_link_state_id(link_state_id);
    header.set_advertising_router(advertising_router);

    return Lsa::LsaRef(nlsa);
}

Lsa::LsaRef
create_summary_lsa(OspfTypes::Version version, uint32_t link_state_id,
		   uint32_t advertising_router, uint32_t mask, uint32_t metric)
{
    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(version);
    Lsa_header& header = snlsa->get_header();

    uint32_t options = compute_options(version, OspfTypes::NORMAL);

    // Set the header fields.
    switch(version) {
    case OspfTypes::V2:
	header.set_options(options);
	snlsa->set_network_mask(mask);
	break;
    case OspfTypes::V3:
	XLOG_WARNING("TBD");
	break;
    }

    header.set_link_state_id(link_state_id);
    header.set_advertising_router(advertising_router);

    snlsa->set_metric(metric);

    return Lsa::LsaRef(snlsa);
}

Lsa::LsaRef
create_summary_lsa(OspfTypes::Version version, uint32_t link_state_id,
		   uint32_t advertising_router, uint32_t metric)
{
    SummaryRouterLsa *srlsa = new SummaryRouterLsa(version);
    Lsa_header& header = srlsa->get_header();

    uint32_t options = compute_options(version, OspfTypes::NORMAL);

    // Set the header fields.
    switch(version) {
    case OspfTypes::V2:
	header.set_options(options);
	break;
    case OspfTypes::V3:
	break;
    }

    header.set_link_state_id(link_state_id);
    header.set_advertising_router(advertising_router);

    srlsa->set_metric(metric);

    return Lsa::LsaRef(srlsa);
}

Lsa::LsaRef
create_external_lsa(OspfTypes::Version version, uint32_t link_state_id,
		    uint32_t advertising_router)
{
    ASExternalLsa *aselsa = new ASExternalLsa(version);
    Lsa_header& header = aselsa->get_header();

    uint32_t options = compute_options(version, OspfTypes::NORMAL);

    // Set the header fields.
    switch(version) {
    case OspfTypes::V2:
	header.set_options(options);
	break;
    case OspfTypes::V3:
	break;
    }

    header.set_link_state_id(link_state_id);
    header.set_advertising_router(advertising_router);

    return Lsa::LsaRef(aselsa);
}

template <typename A> 
bool
verify_routes(TestInfo& info, uint32_t lineno, DebugIO<A>& io, uint32_t routes)
{
    if (routes != io.routing_table_size()) {
	DOUT(info) << "Line number: " << lineno << endl;
	DOUT(info) << "Expecting " << routes << " routes " << "got " <<
	    io.routing_table_size() << endl;
	return false;
    }
    return true;
}

// This is the origin.

Lsa::LsaRef
create_RT6(OspfTypes::Version version)
{
     Lsa::LsaRef lsar = create_router_lsa(version, 6, 6);
     RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
     XLOG_ASSERT(rlsa);

     // Link to RT3
     p2p(version, rlsa, 3, 4, 6);

     // Link to RT5
     p2p(version, rlsa, 5, 6, 6);

     // Link to RT10 XXX need to look at this more carefully.
     p2p(version, rlsa, 10, 11, 7);

     rlsa->encode();

     rlsa->set_self_originating(true);

     return lsar;
}

Lsa::LsaRef
create_RT3(OspfTypes::Version version)
{
     Lsa::LsaRef lsar = create_router_lsa(version, 3, 3);
     RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
     XLOG_ASSERT(rlsa);

     // Link to RT6
     p2p(version, rlsa, 6, 7, 8);

     // Network to N4
     stub(version, rlsa, (4 << 16), 0xffff0000, 2);

     // Network to N3
//      network(version, rlsa, 3, 1);

     rlsa->encode();

     return lsar;
}

// Some of the routers from Figure 2. in RFC 2328. Single area.
// This router is R6.

template <typename A> 
bool
routing1(TestInfo& info, OspfTypes::Version version)
{
    EventLoop eventloop;
    DebugIO<A> io(info, version, eventloop);
    io.startup();
    
    Ospf<A> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    ospf.set_router_id(set_id("0.0.0.6"));

    OspfTypes::AreaID area = set_id("128.16.64.16");

    PeerManager<A>& pm = ospf.get_peer_manager();

    // Create an area
    pm.create_area_router(area, OspfTypes::NORMAL);

    // Create a peer associated with this area.
    const string interface = "eth0";
    const string vif = "vif0";

    A src;
    switch(src.ip_version()) {
    case 4:
	src = "192.150.187.78";
	break;
    case 6:
	src = "2001:468:e21:c800:220:edff:fe61:f033";
	break;
    default:
	XLOG_FATAL("Unknown IP version %d", src.ip_version());
	break;
    }

    OspfTypes::PeerID peerid = pm.
	create_peer(interface, vif, src, OspfTypes::BROADCAST, area);

    // Bring the peering up
    if (!pm.set_state_peer(peerid, true)) {
	DOUT(info) << "Failed enable peer\n";
	return false;
    }

    AreaRouter<A> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    ar->testing_replace_router_lsa(create_RT6(version));

    ar->testing_add_lsa(create_RT3(version));

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    ar->testing_delete_lsa(create_RT3(version));

    // At the time of writing the OSPFv3 routing table computations
    // were not complete, when they are remove this test.
    if (OspfTypes::V2 == version) {
	// At this point there should be a single route in the routing
	// table.
	if (!verify_routes(info, __LINE__, io, 1))
	    return false;
	const uint32_t routes = 1;
	if (routes != io.routing_table_size()) {
	    DOUT(info) << "Expecting " << routes << " routes " << "got " <<
		io.routing_table_size() << endl;
	    return false;
	}
	if (!io.routing_table_verify(IPNet<A>("0.4.0.0/16"),
				    A("0.0.0.7"), 8, false, false)) {
	    DOUT(info) << "Mismatch in routing table\n";
	    return false;
	}
    }

    // Now delete the routes.

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    // Take the peering down
    if (!pm.set_state_peer(peerid, false)) {
	DOUT(info) << "Failed to disable peer\n";
	return false;
    }

    // Delete the peer.
    if (!pm.delete_peer(peerid)) {
	DOUT(info) << "Failed to delete peer\n";
	return false;
    }

    // Delete the area
    if (!pm.destroy_area_router(area)) {
	DOUT(info) << "Failed to delete area\n";
	return false;
    }

    // The routing table should be empty now.
    if (0 != io.routing_table_size()) {
	DOUT(info) << "Expecting no routes " << "got " <<
	    io.routing_table_size() << endl;
	return false;
    }

    return true;
}

// Attempting to reproduce:
// http://www.xorp.org/bugzilla/show_bug.cgi?id=226
bool
routing2(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::RouterID rid = set_id("10.0.8.161");
    ospf.set_router_id(rid);
    
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();

    // Create an area
    pm.create_area_router(area, OspfTypes::NORMAL);

    // Create a peer associated with this area.
    IPv4 src("172.16.1.1");
    const string interface = "eth0";
    const string vif = "vif0";

    OspfTypes::PeerID peerid = pm.
	create_peer(interface, vif, src, OspfTypes::BROADCAST, area);

    // Bring the peering up
    if (!pm.set_state_peer(peerid, true)) {
	DOUT(info) << "Failed enable peer\n";
	return false;
    }

    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("172.16.1.2"), set_id("172.16.1.1"), 1);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);
    
    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("172.16.1.2");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("172.16.1.2"), set_id("172.16.1.2"), 1);
    stub(version, rlsa, set_id("172.16.2.1"), 0xffffffff, 1);
    stub(version, rlsa, set_id("172.16.1.100"), 0xffffffff, 1);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, prid, prid, 0xfffffffc);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(prid);
    nlsa->get_attached_routers().push_back(rid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/
    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 2))
	return false;

    if (!io.routing_table_verify(IPNet<IPv4>("172.16.1.100/32"),
				 IPv4("172.16.1.2"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    if (!io.routing_table_verify(IPNet<IPv4>("172.16.2.1/32"),
				 IPv4("172.16.1.2"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    /*********************************************************/
    ar->testing_delete_lsa(lsar);
    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 0))
	return false;

    /*********************************************************/
    // Create the Network-LSA again as the first one has been invalidated.
    lsar = create_network_lsa(version, prid, prid, 0xfffffffc);
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(prid);
    nlsa->get_attached_routers().push_back(rid);
    lsar->encode();
    ar->testing_add_lsa(lsar);
    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 2))
	return false;

    if (!io.routing_table_verify(IPNet<IPv4>("172.16.1.100/32"),
				 IPv4("172.16.1.2"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    if (!io.routing_table_verify(IPNet<IPv4>("172.16.2.1/32"),
				 IPv4("172.16.1.2"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }
    /*********************************************************/
    ar->testing_delete_lsa(lsar);
    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 0))
	return false;

    /*********************************************************/
    // Take the peering down
    if (!pm.set_state_peer(peerid, false)) {
	DOUT(info) << "Failed to disable peer\n";
	return false;
    }

    // Delete the peer.
    if (!pm.delete_peer(peerid)) {
	DOUT(info) << "Failed to delete peer\n";
	return false;
    }

    // Delete the area
    if (!pm.destroy_area_router(area)) {
	DOUT(info) << "Failed to delete area\n";
	return false;
    }

    // The routing table should be empty now.
    if (!verify_routes(info, __LINE__, io, 0))
	return false;

    return true;
}

inline
bool
type_check(TestInfo& info, string message, uint32_t expected, uint32_t actual)
{
    if (expected != actual) {
	DOUT(info) << message << " should be " << expected << " is " << 
	    actual << endl;
	return false;
    }

    return true;
}

/**
 * Read in LSA database files written by print_lsas.cc and run the
 * routing computation.
 */
template <typename A> 
bool
routing3(TestInfo& info, OspfTypes::Version version, string fname)
{
    if (0 == fname.size()) {
	DOUT(info) << "No filename supplied\n";

	return true;
    }

    EventLoop eventloop;
    DebugIO<A> io(info, version, eventloop);
    io.startup();
    
    Ospf<A> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    Tlv tlv;

    if (!tlv.open(fname, true /* read */)) {
	DOUT(info) << "Failed to open " << fname << endl;
	return false;
    }

    vector<uint8_t> data;
    uint32_t type;

    // Read the version field and check it.
    if (!tlv.read(type, data)) {
	DOUT(info) << "Failed to read " << fname << endl;
	return false;
    }

    if (!type_check(info, "TLV Version", TLV_VERSION, type))
	return false;

    uint32_t tlv_version;
    if (!tlv.get32(data, 0, tlv_version)) {
	return false;
    }

    if (!type_check(info, "Version", TLV_CURRENT_VERSION, tlv_version))
	return false;

    // Read the system info
    if (!tlv.read(type, data)) {
	DOUT(info) << "Failed to read " << fname << endl;
	return false;
    }

    if (!type_check(info, "TLV System info", TLV_SYSTEM_INFO, type))
	return false;

    data.resize(data.size() + 1);
    data[data.size() - 1] = 0;
    
    DOUT(info) << "Built on " << &data[0] << endl;

    // Get OSPF version
    if (!tlv.read(type, data)) {
	DOUT(info) << "Failed to read " << fname << endl;
	return false;
    }
    
    if (!type_check(info, "TLV OSPF Version", TLV_OSPF_VERSION, type))
	return false;

    uint32_t ospf_version;
    if (!tlv.get32(data, 0, ospf_version)) {
	return false;
    }

    if (!type_check(info, "OSPF Version", version, ospf_version))
	return false;

    // OSPF area
    if (!tlv.read(type, data)) {
	DOUT(info) << "Failed to read " << fname << endl;
	return false;
    }
    
    if (!type_check(info, "TLV OSPF Area", TLV_AREA, type))
	return false;
    
    OspfTypes::AreaID area;
    if (!tlv.get32(data, 0, area)) {
	return false;
    }

    PeerManager<A>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<A> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    // The first LSA is this routers Router-LSA.
    if (!tlv.read(type, data)) {
	DOUT(info) << "Failed to read " << fname << endl;
	return false;
    }
    
    if (!type_check(info, "TLV LSA", TLV_LSA, type))
	return false;

    LsaDecoder lsa_decoder(version);
    initialise_lsa_decoder(version, lsa_decoder);

    size_t len = data.size();
    Lsa::LsaRef lsar = lsa_decoder.decode(&data[0], len);

    DOUT(info) << lsar->str() << endl;

    ospf.set_router_id(lsar->get_header().get_advertising_router());

    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Keep reading LSAs until we run out or hit a new area.
    for(;;) {
	if (!tlv.read(type, data))
	    break;
	if (TLV_LSA != type)
	    break;
	size_t len = data.size();
	Lsa::LsaRef lsar = lsa_decoder.decode(&data[0], len);
	ar->testing_add_lsa(lsar);
    }

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    return true;
}

/**
 * This is a similar problem to:
 *	http://www.xorp.org/bugzilla/show_bug.cgi?id=295
 *
 * Two Router-LSAs an a Network-LSA, the router under consideration
 * generated the Network-LSA. The other Router-LSA has the e and b
 * bits set so it must be installed in the routing table to help with
 * the routing computation. An AS-External-LSA generated by the other router.
 */
bool
routing4(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("10.0.1.1");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("10.0.1.6");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer.
    lsar = create_external_lsa(version, set_id("10.20.0.0"), prid);
    ASExternalLsa *aselsa;
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.0.1.6"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/    

    if (info.verbose())
	ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 1))
	return false;

#if	0
    // This route made it to the routing table but not to the RIB
    if (!io.routing_table_verify(IPNet<IPv4>("10.0.1.6/32"),
				 IPv4("10.0.1.6"), 1, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }
#endif

    if (!io.routing_table_verify(IPNet<IPv4>("10.20.0.0/16"),
				 IPv4("10.0.1.6"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }
    
    return true;
}

/**
 * http://www.xorp.org/bugzilla/show_bug.cgi?id=359
 * Used to demonstrate that routes would be introduced from dead LSAs.
 */
bool
routing5(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("10.0.1.1");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("10.0.1.6");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the second peer's Router-LSA
    OspfTypes::RouterID pprid = set_id("10.0.1.8");
    lsar = create_router_lsa(version, pprid, pprid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, pprid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer.
    lsar = create_external_lsa(version, set_id("10.20.0.0"), prid);
    ASExternalLsa *aselsa;
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.0.1.6"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the second peer.
    lsar = create_external_lsa(version, set_id("10.21.0.0"), pprid);
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.0.1.8"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 1))
        return false;

    if (!io.routing_table_verify(IPNet<IPv4>("10.20.0.0/16"),
                                 IPv4("10.0.1.6"), 2, false, false)) {
        DOUT(info) << "Mismatch in routing table\n";
        return false;
    }

    return true;
}

/**
 * http://www.xorp.org/bugzilla/show_bug.cgi?id=374
 * To test various forwarding address variants.
 */
bool
routing6(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("10.0.1.1");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("10.0.1.6");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with peer IP as forwarding
    // address.
    lsar = create_external_lsa(version, set_id("10.20.0.0"), prid);
    ASExternalLsa *aselsa;
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.0.1.6"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with 0.0.0.0 as forwarding
    // address.
    lsar = create_external_lsa(version, set_id("10.21.0.0"), prid);
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("0.0.0.0"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with third router (not in
    // OSPF domain) as forwarding address.
    lsar = create_external_lsa(version, set_id("10.22.0.0"), prid);
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.0.1.8"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 3))
        return false;

    if (!io.routing_table_verify(IPNet<IPv4>("10.20.0.0/16"),
                                 IPv4("10.0.1.6"), 2, false, false)) {
        DOUT(info) << "Mismatch in routing table\n";
        return false;
    }
    if (!io.routing_table_verify(IPNet<IPv4>("10.21.0.0/16"),
				 IPv4("10.0.1.6"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }
    if (!io.routing_table_verify(IPNet<IPv4>("10.22.0.0/16"),
				 IPv4("10.0.1.8"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    return true;
}

/**
 * http://www.xorp.org/bugzilla/show_bug.cgi?id=375
 * To test various forwarding address variants in external LSA originated
 * not by directly connected neighbour.
 * +-------------+ DR          +----------+ DR          +----------+
 * | This router | 10.0.0.0/16 |   prid   | 10.2.0.0/16 |   pprid  |
 * |   10.0.1.1  |-------------| 10.0.1.6 |-------------| 10.2.1.6 |
 * +-------------+             +----------+             +----------+
 *             10.0.1.1   10.0.1.6      10.2.1.1   10.2.1.6
 */
bool
routing7(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("10.0.1.1");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("10.0.1.6");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1);
    transit(version, rlsa, set_id("10.2.1.1"), set_id("10.2.1.1"), 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Router-LSA from peer behind 10.0.1.6.
    OspfTypes::RouterID pprid = set_id("10.2.1.6");
    lsar = create_router_lsa(version, pprid, pprid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("10.2.1.1"), pprid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Network LSA to bind prid and pprid together.
    lsar = create_network_lsa(version, set_id("10.2.1.1"), prid, 0xffff0000);
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(prid);
    nlsa->get_attached_routers().push_back(pprid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with peer IP as forwarding
    // address.
    lsar = create_external_lsa(version, set_id("10.20.0.0"), pprid);
    ASExternalLsa *aselsa;
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.2.1.6"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with 0.0.0.0 as forwarding
    // address.
    lsar = create_external_lsa(version, set_id("10.21.0.0"), pprid);
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("0.0.0.0"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the AS-External-LSA from the peer with third router (not in
    // OSPF domain) as forwarding address.
    lsar = create_external_lsa(version, set_id("10.22.0.0"), pprid);
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffff0000);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("10.2.1.8"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 4))
        return false;

    if (!io.routing_table_verify(IPNet<IPv4>("10.2.0.0/16"),
				 IPv4("10.0.1.6"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }
    if (!io.routing_table_verify(IPNet<IPv4>("10.20.0.0/16"),
                                 IPv4("10.0.1.6"), 3, false, false)) {
        DOUT(info) << "Mismatch in routing table\n";
        return false;
    }
    if (!io.routing_table_verify(IPNet<IPv4>("10.21.0.0/16"),
                                 IPv4("10.0.1.6"), 2, false, false)) {
        DOUT(info) << "Mismatch in routing table\n";
        return false;
    }
    if (!io.routing_table_verify(IPNet<IPv4>("10.22.0.0/16"),
                                 IPv4("10.0.1.6"), 3, false, false)) {
        DOUT(info) << "Mismatch in routing table\n";
        return false;
    }

    return true;
}

/**
 * http://www.xorp.org/bugzilla/show_bug.cgi?id=395
 * Verify that a Summary-LSA with the DefaultDestination is correctly
 * installed in the routing table.
 */
bool
routing8(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("10.0.1.1");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer's Router-LSA
    OspfTypes::RouterID prid = set_id("10.0.1.6");
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Summary-LSA with the DefaultDestination.
    lsar = create_summary_lsa(version, OspfTypes::DefaultDestination, prid,
			      0 /* mask */, 1 /* metric */);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 1))
        return false;

    if (!io.routing_table_verify(IPNet<IPv4>("0.0.0.0/0"),
				 IPv4("10.0.1.6"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    return true;
}

/**
 * The same route can be in the routing table more than once either
 * due to a mis-configuration or simple race.
 * 
 * +------------+                                             
 * |This router |                                             
 * |128.16.64.16|                                             
 * +------------+                                             
 *       | 10.0.0.16     10.0.0.1             10.0.0.2                         
 *     ------------------------------------------------       10.0.0.0/16
 *                             |                     |       
 *                       +----------+         +----------+ 
 *                       |    p1    |         |     p2   | 
 *                       | 0.0.0.1  |         | 0.0.0.2  | 
 *                       +----------+         +----------+ 
 *                             |                     |       
 *     ------------------------------------------------       20.0.0.0/16
 *                       20.0.0.1             20.0.0.2
 *
 * Network 10.0.0.0/16 the Network-LSA is generated by this router.
 * Network 20.0.0.0/16 the Network-LSA is generated by p1.
 * The Router-LSA from p2 is announcing 20.0.0.0/16 as a stub link.
 *
 * An attempt will be made to introduce 20.0.0.0/16 both from the
 * Network-LSA and the the stub link of the Router-LSA.
 */
bool
routing9(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID rid = set_id("128.16.64.16");
    ospf.set_router_id(rid);

    // Create this router's Router-LSA
    Lsa::LsaRef lsar;
    lsar = create_router_lsa(version, rid, rid);
    RouterLsa *rlsa;
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("10.0.0.16"), set_id("10.0.0.16"), 1);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the p1 Router-LSA
    OspfTypes::RouterID p1_rid = set_id("0.0.0.1");
    lsar = create_router_lsa(version, p1_rid, p1_rid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("10.0.0.16"), set_id("10.0.0.1"), 1);
    transit(version, rlsa, set_id("20.0.0.1"), set_id("20.0.0.1"), 1);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the p2 Router-LSA
    OspfTypes::RouterID p2_rid = set_id("0.0.0.2");
    lsar = create_router_lsa(version, p2_rid, p2_rid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, set_id("10.0.0.16"), set_id("10.0.0.2"), 1);
    stub(version, rlsa, set_id("20.0.0.0"), 0xffff0000, 1);
    // This Router-LSA has not yet transitioned to the the transit below.
//     transit(version, rlsa, set_id("20.0.0.1"), set_id("20.0.0.2"), 1);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue on the
    // 10.0.0.0/16 network.
    lsar = create_network_lsa(version, set_id("10.0.0.16"), rid, 0xffff0000);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(p1_rid);
    nlsa->get_attached_routers().push_back(p2_rid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue on the
    // 20.0.0.0/16 network.
    lsar = create_network_lsa(version, set_id("20.0.0.1"), p1_rid, 0xffff0000);
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(p1_rid);
    nlsa->get_attached_routers().push_back(p2_rid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    /*********************************************************/

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 1))
        return false;

    if (!io.routing_table_verify(IPNet<IPv4>("20.0.0.0/16"),
				 IPv4("10.0.0.1"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    return true;
}

bool
routing10(TestInfo& info)
{
    OspfTypes::Version version = OspfTypes::V2;

    EventLoop eventloop;
    DebugIO<IPv4> io(info, version, eventloop);
    io.startup();

    Ospf<IPv4> ospf(version, eventloop, &io);
    ospf.trace().all(info.verbose());
    OspfTypes::AreaID area = set_id("0.0.0.0");

    PeerManager<IPv4>& pm = ospf.get_peer_manager();
    pm.create_area_router(area, OspfTypes::NORMAL);
    AreaRouter<IPv4> *ar = pm.get_area_router(area);
    XLOG_ASSERT(ar);

    OspfTypes::RouterID prid = set_id("10.10.10.149");
    ospf.set_router_id(prid);
    Lsa::LsaRef lsar;
    RouterLsa *rlsa;

    // Set testing to true to force the 2-Way test to return true.
    ospf.set_testing(true);

    // Create the peer 1 (DR) Router-LSA
    OspfTypes::RouterID rid = set_id("10.10.10.125");
    lsar = create_router_lsa(version, rid, rid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, rid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);
    
    // Create this router's Router-LSA
    lsar = create_router_lsa(version, prid, prid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, prid, 1 /* metric */);
    lsar->encode();
    lsar->set_self_originating(true);
    ar->testing_replace_router_lsa(lsar);

    // Create the peer 2 Router-LSA
    OspfTypes::RouterID pprid = set_id("10.10.10.18");
    lsar = create_router_lsa(version, pprid, pprid);
    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    transit(version, rlsa, rid, pprid, 1);
    rlsa->set_e_bit(true);
    rlsa->set_b_bit(true);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Create the Network-LSA that acts as the binding glue.
    lsar = create_network_lsa(version, rid, rid, 0xffffff00);
    NetworkLsa *nlsa;
    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    nlsa->get_attached_routers().push_back(rid);
    nlsa->get_attached_routers().push_back(prid);
    nlsa->get_attached_routers().push_back(pprid);
    lsar->encode();
    ar->testing_add_lsa(lsar);

    // Peer 2 originates External LSA
    lsar = create_external_lsa(version, set_id("10.20.30.0"), pprid);
    ASExternalLsa *aselsa;
    aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    aselsa->set_network_mask(0xffffff00);
    aselsa->set_metric(1);
    aselsa->set_forwarding_address_ipv4(IPv4("0.0.0.0"));
    lsar->encode();
    ar->testing_add_lsa(lsar);

    if (info.verbose())
        ar->testing_print_link_state_database();
    ar->testing_routing_total_recompute();

    if (!verify_routes(info, __LINE__, io, 1))
        return false;
    if (!io.routing_table_verify(IPNet<IPv4>("10.20.30.0/24"),
		                 IPv4("10.10.10.18"), 2, false, false)) {
	DOUT(info) << "Mismatch in routing table\n";
	return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    string fname =t.get_optional_args("-f", "--filename", "lsa database");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"r1V2", callback(routing1<IPv4>, OspfTypes::V2)},
	{"r1V3", callback(routing1<IPv6>, OspfTypes::V3)},
	{"r2", callback(routing2)},
	{"r3V2", callback(routing3<IPv4>, OspfTypes::V2, fname)},
	{"r3V3", callback(routing3<IPv6>, OspfTypes::V3, fname)},
	{"r4", callback(routing4)},
	{"r5", callback(routing5)},
	{"r6", callback(routing6)},
	{"r7", callback(routing7)},
 	{"r8", callback(routing8)},
 	{"r9", callback(routing9)},
 	{"r10", callback(routing10)},
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return t.exit();
}
