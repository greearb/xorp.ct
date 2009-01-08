// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/contrib/olsr/olsr.hh,v 1.3 2008/10/02 21:56:35 bms Exp $

#ifndef __OLSR_OLSR_HH__
#define __OLSR_OLSR_HH__

#include "olsr_types.hh"
#include "exceptions.hh"
#include "policy_varrw.hh"
#include "io.hh"
#include "message.hh"
#include "link.hh"
#include "neighbor.hh"
#include "twohop.hh"
#include "face.hh"
#include "face_manager.hh"
#include "neighborhood.hh"
#include "topology.hh"
#include "external.hh"

#include "libxorp/trie.hh"
#include "libproto/spt.hh"

#include "vertex.hh"
#include "route_manager.hh"

#include "trace.hh"

#include "libxorp/profile.hh"
#include "profile_vars.hh"

/**
 * @short An OLSR routing process.
 *
 * IO is performed by the associated class IO.
 */
class Olsr {
  public:
    Olsr(EventLoop& eventloop, IO* io);

    /**
     * @return true if this routing process is running.
     */
    inline bool running() { return _io->status() != SERVICE_SHUTDOWN; }

    /**
     * Determine routing process status.
     *
     * @param reason a text description of the current process status.
     * @return status of the routing process.
     */
    ProcessStatus status(string& reason);

    /**
     * Shut down this routing process.
     */
    void shutdown();

    /**
     * Callback method to: receive a datagram.
     *
     * All packets for OLSR are received through this interface. All good
     * packets are sent to the face manager which verifies that the packet
     * is expected, and is wholly responsible for further processing.
     */
    void receive(const string& interface, const string& vif,
	IPv4 dst, uint16_t dport, IPv4 src, uint16_t sport,
	uint8_t* data, uint32_t len);

    /**
     * Transmit a datagram.
     */
    bool transmit(const string& interface, const string& vif,
	const IPv4& dst, const uint16_t& dport,
	const IPv4& src, const uint16_t& sport,
	uint8_t* data, const uint32_t& len);

    /**
     * Register vif status callback with the FEA.
     *
     * @param cb the callback to register.
     */
    inline void register_vif_status(IO::VifStatusCb cb) {
	_io->register_vif_status(cb);
    }

    /**
     * Register address status callback with the FEA.
     *
     * @param cb the callback to register.
     */
    inline void register_address_status(IO::AddressStatusCb cb) {
	_io->register_address_status(cb);
    }

    inline EventLoop& get_eventloop() {
	return _eventloop;
    }

    inline FaceManager& face_manager() {
	return _fm;
    }

    inline Neighborhood& neighborhood() {
	return _nh;
    }

    inline TopologyManager& topology_manager() {
	return _tm;
    }

    inline ExternalRoutes& external_routes() {
	return _er;
    }

    inline RouteManager& route_manager() {
	return _rm;
    }

    /**
     * Set the main address for the node.
     *
     * @param addr the main address to set.
     * @return true if the main address was set successfully.
     */
    inline bool set_main_addr(const IPv4& addr) {
	return face_manager().set_main_addr(addr);
    }

    /**
     * @return the main address.
     */
    inline IPv4 get_main_addr() {
	return face_manager().get_main_addr();
    }

    /**
     * Set the MPR_COVERAGE for the node.
     *
     * @param mpr_coverage the new value of MPR_COVERAGE.
     * @return true if MPR_COVERAGE was set successfully.
     */
    inline bool set_mpr_coverage(const uint32_t mpr_coverage) {
	neighborhood().set_mpr_coverage(mpr_coverage);
	return true;
    }

    /**
     * @return the value of the MPR_COVERAGE variable.
     */
    inline uint32_t get_mpr_coverage() {
	return neighborhood().mpr_coverage();
    }

    /**
     * Set the TC_REDUNDANCY for the node.
     *
     * @param tc_redundancy the new value of TC_REDUNDANCY.
     * @return true if TC_REDUNDANCY was set successfully.
     */
    inline bool set_tc_redundancy(const uint32_t tc_redundancy) {
	neighborhood().set_tc_redundancy(tc_redundancy);
	return true;
    }

    /**
     * @return the value of the TC_REDUNDANCY variable.
     */
    inline uint32_t get_tc_redundancy() {
	return neighborhood().get_tc_redundancy();
    }

    /**
     * Set the HELLO_INTERVAL.
     *
     * @param interval the new value of HELLO_INTERVAL.
     * @return true if HELLO_INTERVAL was set successfully.
     */
    inline bool set_hello_interval(const TimeVal& interval) {
	face_manager().set_hello_interval(interval);
	return true;
    }

    /**
     * @return the value of the HELLO_INTERVAL variable.
     */
    inline TimeVal get_hello_interval() {
	return face_manager().get_hello_interval();
    }

    /**
     * Set the MID_INTERVAL.
     *
     * @param interval the new value of MID_INTERVAL.
     * @return true if MID_INTERVAL was set successfully.
     */
    inline bool set_mid_interval(const TimeVal& interval) {
	face_manager().set_mid_interval(interval);
	return true;
    }

    /**
     * @return the value of the MID_INTERVAL variable.
     */
    inline TimeVal get_mid_interval() {
	return face_manager().get_mid_interval();
    }

    /**
     * Set the REFRESH_INTERVAL.
     *
     * @param interval the new refresh interval.
     * @return true if the refresh interval was set successfully.
     */
    inline bool set_refresh_interval(const TimeVal& interval) {
	neighborhood().set_refresh_interval(interval);
	return true;
    }

    /**
     * @return the value of the REFRESH_INTERVAL protocol variable.
     */
    inline TimeVal get_refresh_interval() {
	return neighborhood().get_refresh_interval();
    }

    /**
     * Set the TC interval for the node.
     *
     * @param interval the new TC interval.
     * @return true if the TC interval was set successfully.
     */
    inline bool set_tc_interval(TimeVal interval) {
	neighborhood().set_tc_interval(interval);
	return true;
    }

    /**
     * @return the value of the TC_INTERVAL protocol variable.
     */
    inline TimeVal get_tc_interval() {
	return neighborhood().get_tc_interval();
    }

    /**
     * Set the WILLINGNESS.
     *
     * @param willingness the new willingness-to-forward.
     * @return true if the willingness was set successfully.
     */
    inline bool set_willingness(const OlsrTypes::WillType willingness) {
	neighborhood().set_willingness(willingness);
	return true;
    }

    /**
     * @return the WILLINGNESS protocol variable.
     */
    inline OlsrTypes::WillType get_willingness() {
	return neighborhood().willingness();
    }

    /**
     * Set the HNA interval.
     *
     * @param interval the new HNA interval.
     * @return true if the HNA interval was set successfully.
     */
    inline bool set_hna_interval(const TimeVal& interval) {
	external_routes().set_hna_interval(interval);
	return true;
    }

    /**
     * @return the value of the HNA_INTERVAL protocol variable.
     */
    inline TimeVal get_hna_interval() {
	return external_routes().get_hna_interval();
    }

    /**
     * Set the DUP_HOLD_TIME.
     *
     * @param interval the new duplicate message hold time
     * @return true if the DUP_HOLD_TIME was set successfully.
     */
    inline bool set_dup_hold_time(const TimeVal& interval) {
	face_manager().set_dup_hold_time(interval);
	return true;
    }

    /**
     * @return the value of the DUP_HOLD_TIME protocol variable.
     */
    inline TimeVal get_dup_hold_time() {
	return face_manager().get_dup_hold_time();
    }

    /**
     * Bind OLSR to an interface.
     *
     * @param interface the name of the interface to listen on.
     * @param vif the name of the vif to listen on.
     * @param local_addr the address to listen on.
     * @param local_port the address on the interface to listen on.
     * @param all_nodes_addr the address to transmit to.
     * @param all_nodes_port the port to transmit to.
     * @return true if the address was bound successfully.
     */
    bool bind_address(const string& interface,
		      const string& vif,
		      const IPv4& local_addr,
		      const uint32_t& local_port,
		      const IPv4& all_nodes_addr,
		      const uint32_t& all_nodes_port);

    /**
     * Unbind OLSR from an interface.
     *
     * OLSR may only be bound to a single address on an interface,
     * therefore there is no 'address' argument to this method.
     *
     * @param interface the name of the interface to unbind.
     * @param vif the name of the vif to unbind.
     * @return true if the interface was unbound successfully.
     */
    bool unbind_address(const string& interface, const string& vif);

    /**
     * Enable I/O for OLSR on a given address.
     *
     * Proxy for class IO.
     *
     * @param interface the name of the interface to listen on.
     * @param vif the name of the vif to listen on.
     * @param address the name of the address to listen on.
     * @param port the name of the port to listen on.
     * @param all_nodes_address the name of the address to transmit to.
     * @return true if the address was enabled successfully.
     */
    bool enable_address(const string& interface, const string& vif,
		        const IPv4& address, const uint16_t& port,
			const IPv4& all_nodes_address);

    /**
     * Disable I/O for OLSR on a given address.
     *
     * Proxy for class IO.
     *
     * @param interface the name of the interface to listen on.
     * @param vif the name of the vif to listen on.
     * @param address the name of the address to listen on.
     * @param port the name of the port to listen on.
     * @return true if the address was enabled successfully.
     */
    bool disable_address(const string& interface, const string& vif,
		         const IPv4& address, const uint16_t& port);

    /**
     * Enable or disable an interface for OLSR.
     *
     * @param interface the name of the interface to enable/disable.
     * @param vif the name of the vif to enable/disable.
     * @param enabled the new enable state of the interface.
     * @return true if the interface state was set to @param enabled
     *         successfully.
     */
    bool set_interface_enabled(const string& interface, const string& vif,
			       const bool enabled);

    /**
     * Determine if an interface is enabled for OLSR.
     *
     * @param interface the name of the interface to examine.
     * @param vif the name of the vif to examine.
     * @param enabled output variable will be set to the enable state
     *                of the interface.
     * @return true if the enable state was retrieved successfully.
     */
    bool get_interface_enabled(const string& interface, const string& vif,
			       bool& enabled);

    /**
     * Get an interface's statistics.
     *
     * @param interface the name of the interface to examine.
     * @param vif the name of the vif to examine.
     * @param stats output variable will be set to the interface stats.
     * @return true if the stats werre retrieved successfully.
     */
    bool get_interface_stats(const string& interface, const string& vif,
			     FaceCounters& stats);

    /**
     * Set the cost on an interface.
     *
     * @param interface the name of the interface to set cost for.
     * @param vif the name of the vif to set cost for.
     * @param cost the new cost of the interface.
     * @return true if the interface cost was set successfully.
     */
    inline bool set_interface_cost(const string& interface,
	const string& vif, int cost)
    {
	OlsrTypes::FaceID faceid = face_manager().get_faceid(interface, vif);
	return face_manager().set_interface_cost(faceid, cost);
    }

    /**
     * Get the MTU of an interface.
     *
     * @param interface the name of the interface to get MTU for.
     * @return the MTU of the interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * Get the primary IPv4 broadcast address configured for this
     * interface/vif/address.
     *
     * @param interface the interface to get broadcast address for.
     * @param vif the vif to get broadcast address for.
     * @param address the IPv4 interface address.
     * @param bcast_address (out argument) the IPv4 broadcast address.
     * @return true if the broadcast address was obtained successfully.
     */
    bool get_broadcast_address(const string& interface, const string& vif,
			       const IPv4& address, IPv4& bcast_address);

    /**
     * Test whether this interface/vif is broadcast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is broadcast capable, otherwise false.
     */
    bool is_vif_broadcast_capable(const string& interface,
	const string& vif);

    /**
     * Test whether this interface/vif is multicast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is multicast capable, otherwise false.
     */
    bool is_vif_multicast_capable(const string& interface,
	const string& vif);

    /**
     * Add route.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     */
    bool add_route(IPv4Net net, IPv4 nexthop, uint32_t nexthop_id,
		   uint32_t metric, const PolicyTags& policytags);

    /**
     * Replace route.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     */
    bool replace_route(IPv4Net net, IPv4 nexthop, uint32_t nexthop_id,
		       uint32_t metric, const PolicyTags& policytags);

    /**
     * Delete route.
     *
     * @param net network
     */
    bool delete_route(IPv4Net net);

    /**
     * Configure a policy filter
     *
     * @param filter Id of filter to configure.
     * @param conf Configuration of filter.
     */
    void configure_filter(const uint32_t& filter, const string& conf);

    /**
     * Reset a policy filter.
     *
     * @param filter Id of filter to reset.
     */
    void reset_filter(const uint32_t& filter);

    /**
     * Push routes through policy filters for re-filtering.
     */
    void push_routes();

    /**
     * Originate an external route in HNA.
     *
     * @param net network prefix to originate.
     * @param nexthop the next hop. Ignored by HNA.
     * @param metric external metric for this route.
     * @param policytags the policy tags associated with this route.
     * @return true if the route was originated successfully.
     */
    bool originate_external_route(const IPv4Net& net,
				  const IPv4& nexthop,
				  const uint32_t& metric,
				  const PolicyTags& policytags);

    /**
     * Withdraw an external route from HNA.
     *
     * @param net network prefix to withdraw.
     * @return true if the route was withdrawn successfully.
     */
    bool withdraw_external_route(const IPv4Net& net);

    /**
     * @return a reference to the policy filters
     */
    PolicyFilters& get_policy_filters() { return _policy_filters; }

    /**
     * @return a reference to the tracing variables
     */
    Trace& trace() { return _trace; }

    /**
     * @return a reference to the profiler
     */
    //Profile& profile() { return _profile; }

    /**
     * Clear protocol databases.
     *
     * A full routing recomputation should be triggered by this operation.
     * This specifically clears links, MID, TC, learned HNA entries,
     * and the duplicate message set only.
     */
    bool clear_database();

  private:
    EventLoop&		_eventloop;
    IO*			_io;

    FaceManager		_fm;
    Neighborhood	_nh;
    TopologyManager	_tm;
    ExternalRoutes	_er;
    RouteManager	_rm;

    string		_reason;
    ProcessStatus	_process_status;

    PolicyFilters	_policy_filters;

    /**
     * Trace variables.
     */
    Trace		_trace;

    /**
     * Profiler instance.
     */
    //Profile		_profile;

};

#endif // __OLSR_OLSR_HH__
