// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// You may also (at your option) redistribute this software and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// $XORP: xorp/ospfd/xorp/ospf_config.h,v 1.2 2003/03/10 23:20:46 hodson Exp $

#ifndef __XORP_OSPF_XORP_XRL_TARGET_HH__
#define __XORP_OSPF_XORP_XRL_TARGET_HH__

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_router.hh"
#include "xrl/targets/ospf_base.hh"

class XrlOspfTarget : public XrlOspfTargetBase {
public:
    XrlOspfTarget(EventLoop& e, XrlRouter& r, XorpOspfd& xo, OSPF** ppo) : 
	XrlOspfTargetBase(&r), _eventloop(e), _xorp_ospfd(xo), _pp_ospf(ppo) 
    {}
    ~XrlOspfTarget() {}

    /* Common Interface methods -------------------------------------------- */

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string&	reason);

    /* OSPFD Global Configuration ------------------------------------------ */

     /**
      *  Set router id
      */
    XrlCmdError ospf_0_1_set_router_id(
	// Input values, 
	const uint32_t&	id);

    /**
     *  Get router id
     */
    XrlCmdError ospf_0_1_get_router_id(
	// Output values, 
	uint32_t&	id);

    /**
     *  Set maximum number of AS-external LSA's
     */
    XrlCmdError ospf_0_1_set_lsdb_limit(
	// Input values, 
	const int32_t&	limit);

    /**
     *  Get maximum number of AS-external LSA's
     */
    XrlCmdError ospf_0_1_get_lsdb_limit(
	// Output values, 
	int32_t&	limit);

    /**
     *  Enable / disable MOSPF
     */
    XrlCmdError ospf_0_1_set_mospf(
	// Input values, 
	const bool&	enabled);

    /**
     *  Get MOSPF enabled stated
     */
    XrlCmdError ospf_0_1_get_mospf(
	// Output values, 
	bool&	enabled);

    /**
     *  Enable / Disable Inter-area multicast
     */
    XrlCmdError ospf_0_1_set_interarea_mc(
	// Input values, 
	const bool&	enabled);

    XrlCmdError ospf_0_1_get_interarea_mc(
	// Output values, 
	bool&	enabled);

    /**
     *  Set time to exit overflow state
     */
    XrlCmdError ospf_0_1_set_overflow_interval(
	// Input values, 
	const int32_t&	ovfl_int);

    XrlCmdError ospf_0_1_get_overflow_interval(
	// Output values, 
	int32_t&	ovfl_int);

    /**
     *  Set flood rate - self orig per second
     */
    XrlCmdError ospf_0_1_set_flood_rate(
	// Input values, 
	const int32_t&	rate);

    XrlCmdError ospf_0_1_get_flood_rate(
	// Output values, 
	int32_t&	rate);

    /**
     *  Set back-to-back retransmissions
     */
    XrlCmdError ospf_0_1_set_max_rxmt_window(
	// Input values, 
	const uint32_t&	window);

    XrlCmdError ospf_0_1_get_max_rxmt_window(
	// Output values, 
	uint32_t&	window);

    /**
     *  Set maximum simultaneous DB exchanges
     */
    XrlCmdError ospf_0_1_set_max_dds(
	// Input values, 
	const uint32_t&	max_dds);

    XrlCmdError ospf_0_1_get_max_dds(
	// Output values, 
	uint32_t&	max_dds);

    /**
     *  Set rate to refresh DoNotAge LSAs
     */
    XrlCmdError ospf_0_1_set_lsa_refresh_rate(
	// Input values, 
	const uint32_t&	rate);

    XrlCmdError ospf_0_1_get_lsa_refresh_rate(
	// Output values, 
	uint32_t&	rate);

    /**
     *  Set the maximum number of point-to-point links that will become
     *  adjacent to a particular neighbor. If there is no limit then value is
     *  zero.
     */
    XrlCmdError ospf_0_1_set_p2p_adj_limit(
	// Input values, 
	const uint32_t&	max_adj);

    XrlCmdError ospf_0_1_get_p2p_adj_limit(
	// Output values, 
	uint32_t&	max_adj);

    /**
     *  Set randomized LSA refreshes
     */
    XrlCmdError ospf_0_1_set_random_refresh(
	// Input values, 
	const bool&	enabled);

    XrlCmdError ospf_0_1_get_random_refresh(
	// Output values, 
	bool&	enabled);

    /* --------------------------------------------------------------------- */
    /* OSPFD Area Configuration */

    /**
     *  Create or configure area
     */
    XrlCmdError ospf_0_1_add_or_configure_area(
	// Input values, 
	const uint32_t&	area_id, 
	const bool&	is_stub, 
	const uint32_t&	default_cost, 
	const bool&	import_summary_routes);

    /**
     *  Delete area
     */
    XrlCmdError ospf_0_1_delete_area(
	// Input values, 
	const uint32_t&	area_id);

    /**
     *  Query area options
     */
    XrlCmdError ospf_0_1_query_area(
	// Input values, 
	const uint32_t&	area_id, 
	// Output values, 
	bool&		is_stub, 
	uint32_t&	default_cost, 
	bool&		import_summary_routes);

    /**
     *  @param area_ids list of value area_ids. Each id is a u32.
     */
    XrlCmdError ospf_0_1_list_area_ids(
	// Output values, 
	XrlAtomList&	area_ids);

    /* --------------------------------------------------------------------- */
    /* OSPFD Aggregate Configuration */

    /**
     *  Add or configure aggregate.
     *  
     *  @param area_id id of area aggregate belongs to.
     *  
     *  @param network network identifier.
     *  
     *  @param netmask netmask identifier.
     *  
     *  @param suppress_advertisement of aggregate.
     */
    XrlCmdError ospf_0_1_add_or_configure_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const bool&	suppress_advertisement);

    /**
     *  Delete aggregate identified by area_id, network, and netmask
     */
    XrlCmdError ospf_0_1_delete_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask);

    /**
     *  Query aggregate identified by area_id, network, and netmask
     */
    XrlCmdError ospf_0_1_query_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	// Output values, 
	bool&		suppress_advertisement);

    /**
     *  Return list of aggregate identifiers for area identified by area_id.
     *  Two lists are returned, the nth elements in each list comprise the
     *  tuple (network,netmask) that uniquely identifies the aggregate within
     *  the area.
     */
    XrlCmdError ospf_0_1_list_aggregates(
	// Input values, 
	const uint32_t&	area_id, 
	// Output values, 
	XrlAtomList&	networks, 
	XrlAtomList&	netmasks);

    /* --------------------------------------------------------------------- */
    /* OSPFD Host Configuration */

    /**
     *  Add or configure host routes.
     *  
     *  @param area_id host is to be advertised in.
     *  
     *  @param cost metric associated with host (0-65535).
     */
    XrlCmdError ospf_0_1_add_or_configure_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id, 
	const uint32_t&	cost);

    /**
     *  Delete host identified by network and netmask
     */
    XrlCmdError ospf_0_1_delete_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id);

    /**
     *  Query host identified by network and netmask
     */
    XrlCmdError ospf_0_1_query_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id, 
	// Output values, 
	uint32_t&	cost);

    /**
     *  Return list of host identifiers for area identified by area_id. Two
     *  lists are returned, the nth elements in each list comprise the tuple
     *  (network,netmask) that uniquely identifies the host within the area.
     */
    XrlCmdError ospf_0_1_list_hosts(
	// Input values, 
	const uint32_t&	area_id, 
	// Output values, 
	XrlAtomList&	networks, 
	XrlAtomList&	netmasks);

    /* --------------------------------------------------------------------- */
    /* OSPFD Link Configuration */

    XrlCmdError ospf_0_1_add_vlink(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id);

    XrlCmdError ospf_0_1_delete_vlink(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id);

    XrlCmdError ospf_0_1_vlink_set_transmit_delay(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	delay_secs);

    XrlCmdError ospf_0_1_vlink_get_transmit_delay(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	delay_secs);

    XrlCmdError ospf_0_1_vlink_set_retransmit_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_vlink_get_retransmit_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_vlink_set_hello_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_vlink_get_hello_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_vlink_set_router_dead_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_vlink_get_router_dead_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs);

    /**
     *  @param type is one of "none", "cleartext", "md5"
     */
    XrlCmdError ospf_0_1_vlink_set_authentication(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const string&	type, 
	const string&	key);

    XrlCmdError ospf_0_1_vlink_get_authentication(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	string&	type, 
	string&	key);

    /**
     *  Return list of neighbour id's (unsigned 32 bit values)
     */
    XrlCmdError ospf_0_1_list_vlinks(
	// Input values, 
	const uint32_t&	transit_id, 
	// Output values, 
	XrlAtomList&	neighbor_id);

    /* --------------------------------------------------------------------- */
    /* OSPFD External Route Configuration */

    XrlCmdError ospf_0_1_add_or_configure_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway, 
	const uint32_t&	type, 
	const uint32_t&	cost, 
	const bool&	multicast, 
	const uint32_t&	external_route_tag);

    XrlCmdError ospf_0_1_delete_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway);

    XrlCmdError ospf_0_1_query_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway, 
	// Output values, 
	uint32_t&	type, 
	uint32_t&	cost, 
	bool&		multicast, 
	uint32_t&	external_route_tag);

    XrlCmdError ospf_0_1_list_external_routes(
	// Input values, 
	const IPv4Net&	network, 
	// Output values, 
	XrlAtomList&	gateways);

    /* --------------------------------------------------------------------- */
    /* Interface configuration */

    XrlCmdError ospf_0_1_add_interface(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	if_index, 
	const uint32_t&	area_id, 
	const uint32_t&	cost, 
	const uint32_t&	mtu, 
	const string&	type, 
	const bool&	ondemand, 
	const bool&	passive);

    XrlCmdError ospf_0_1_interface_set_if_index(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	index);

    XrlCmdError ospf_0_1_interface_get_if_index(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	index);

    XrlCmdError ospf_0_1_interface_set_area_id(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	area_id);

    XrlCmdError ospf_0_1_interface_get_area_id(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	area_id);

    XrlCmdError ospf_0_1_interface_set_cost(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	cost);

    XrlCmdError ospf_0_1_interface_get_cost(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	cost);

    XrlCmdError ospf_0_1_interface_set_mtu(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	mtu);

    XrlCmdError ospf_0_1_interface_get_mtu(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	mtu);

    XrlCmdError ospf_0_1_interface_set_type(
	// Input values, 
	const string&	identifier, 
	const string&	type);

    XrlCmdError ospf_0_1_interface_get_type(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&	type);

    XrlCmdError ospf_0_1_interface_set_dr_priority(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	dr_priority);

    XrlCmdError ospf_0_1_interface_get_dr_priority(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	dr_priority);

    XrlCmdError ospf_0_1_interface_set_transit_delay(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	delay_secs);

    XrlCmdError ospf_0_1_interface_get_transit_delay(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	delay_secs);

    XrlCmdError ospf_0_1_interface_set_retransmit_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_get_retransmit_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_set_router_dead_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_get_router_dead_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_set_poll_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_get_poll_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs);

    XrlCmdError ospf_0_1_interface_set_authentication(
	// Input values, 
	const string&	identifier, 
	const string&	type, 
	const string&	key);

    XrlCmdError ospf_0_1_interface_get_authentication(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&	type, 
	string&	key);

    XrlCmdError ospf_0_1_interface_set_multicast_forwarding(
	// Input values, 
	const string&	identifier, 
	const string&	type);

    XrlCmdError ospf_0_1_interface_get_multicast_forwarding(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&		type);

    XrlCmdError ospf_0_1_interface_set_on_demand(
	// Input values, 
	const string&	identifier, 
	const bool&	on_demand);

    XrlCmdError ospf_0_1_interface_get_on_demand(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	on_demand);

    XrlCmdError ospf_0_1_interface_set_passive(
	// Input values, 
	const string&	identifier, 
	const bool&	passive);

    XrlCmdError ospf_0_1_interface_get_passive(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	passive);

    XrlCmdError ospf_0_1_interface_set_igmp(
	// Input values, 
	const string&	identifier, 
	const bool&	enabled);

    XrlCmdError ospf_0_1_interface_get_igmp(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	enabled);

    XrlCmdError ospf_0_1_delete_interface(
	// Input values, 
	const string&	identifier);

    XrlCmdError ospf_0_1_list_interfaces(
	// Output values, 
	XrlAtomList&	identifiers);

    /* --------------------------------------------------------------------- */
    /* Interface MD5 configuration */

    XrlCmdError ospf_0_1_interface_add_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id, 
	const string&	md5key, 
	const string&	start_receive, 
	const string&	stop_receive, 
	const string&	start_transmit, 
	const string&	stop_transmit);

    XrlCmdError ospf_0_1_interface_get_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id, 
	// Output values, 
	string&	md5key, 
	string&	start_receive, 
	string&	stop_receive, 
	string&	start_transmit, 
	string&	stop_transmit);

    XrlCmdError ospf_0_1_interface_delete_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id);

    XrlCmdError ospf_0_1_interface_list_md5_keys(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	XrlAtomList&	key_ids);

    /* --------------------------------------------------------------------- */
    /* Static Neighbor configuration */
    
    XrlCmdError ospf_0_1_add_neighbor(
	// Input values, 
	const IPv4&	nbr_addr, 
	const bool&	dr_eligible);

    XrlCmdError ospf_0_1_get_neighbor(
	// Input values, 
	const IPv4&	nbr_addr, 
	// Output values, 
	bool&	dr_eligible);

    XrlCmdError ospf_0_1_delete_neighbor(
	// Input values, 
	const IPv4&	nbr_addr);

    XrlCmdError ospf_0_1_list_neighbors(
	// Output values, 
	XrlAtomList&	nbr_addrs);

protected:
    inline bool ospf_ready() const { return (*_pp_ospf != 0); }
    inline OSPF* ospf() { return *_pp_ospf; }

    EventLoop& _eventloop;
    XorpOspfd& _xorp_ospfd;
    OSPF**     _pp_ospf;
};

#endif // __XORP_OSPF_XORP_XRL_TARGET_HH__
