// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/ospf/ospf.hh,v 1.107 2007/03/22 01:44:44 atanu Exp $

#ifndef __OSPF_OSPF_HH__
#define __OSPF_OSPF_HH__

/**
 * OSPF Types
 */
struct OspfTypes {

    /**
     * The OSPF version.
     */
    enum Version {V2 = 2, V3 = 3};
    
    /**
     * The type of an OSPF packet.
     */
    typedef uint16_t Type;

    /**
     * Router ID.
     */
    typedef uint32_t RouterID;

    /**
     * Area ID.
     */
    typedef uint32_t AreaID;

    /**
     * Link Type
     */
    enum LinkType {
	PointToPoint,
	BROADCAST,
	NBMA,
	PointToMultiPoint,
	VirtualLink
    };

    /**
     * Authentication type: OSPFv2 standard header.
     */
    typedef uint16_t AuType;
    static const AuType NULL_AUTHENTICATION = 0;
    static const AuType SIMPLE_PASSWORD = 1;
    static const AuType CRYPTOGRAPHIC_AUTHENTICATION = 2;

    /**
     * Area Type
     */
    enum AreaType {
	NORMAL,		// Normal Area
	STUB,		// Stub Area
	NSSA,		// Not-So-Stubby Area
    };

    /**
     * Routing Entry Type.
     */
    enum VertexType {
	Router,
	Network
    };

    /**
     * NSSA Translator Role.
     */
    enum NSSATranslatorRole {
	ALWAYS,
	CANDIDATE
    };

    /**
     * NSSA Translator State.
     */
    enum NSSATranslatorState {
	ENABLED,
	ELECTED,
	DISABLED,
    };

    /**
     * The AreaID for the backbone area.
     */
    static const AreaID BACKBONE = 0;

    /**
     * An opaque handle that identifies a peer.
     */
    typedef uint32_t PeerID;

    /**
     * An opaque handle that identifies a neighbour.
     */
    typedef uint32_t NeighbourID;

    /**
     * The IP protocol number used by OSPF.
     */
    static const uint16_t IP_PROTOCOL_NUMBER = 89;

    /**
     * An identifier meaning all peers. No single peer can have this
     * identifier.
     */
    static const PeerID ALLPEERS = 0;

    /** 
     * An identifier meaning all neighbours. No single neighbour can
     * have this identifier.
     */
    static const NeighbourID ALLNEIGHBOURS = 0;

    /**
     * An interface ID that will never be allocated OSPFv3 only.
     */
    static const uint32_t UNUSED_INTERFACE_ID = 0;

    /**
     *
     * The maximum time between distinct originations of any particular
     * LSA.  If the LS age field of one of the router's self-originated
     * LSAs reaches the value LSRefreshTime, a new instance of the LSA
     * is originated, even though the contents of the LSA (apart from
     * the LSA header) will be the same.  The value of LSRefreshTime is
     * set to 30 minutes.
     */
    static const uint32_t LSRefreshTime = 30 * 60;

    /**
     * The minimum time between distinct originations of any particular
     * LSA.  The value of MinLSInterval is set to 5 seconds.
     */
    static const uint32_t MinLSInterval = 5;

    /**
     * For any particular LSA, the minimum time that must elapse
     * between reception of new LSA instances during flooding. LSA
     * instances received at higher frequencies are discarded. The
     * value of MinLSArrival is set to 1 second.
     */
    static const uint32_t MinLSArrival = 1;

    /**
     * The maximum age that an LSA can attain. When an LSA's LS age
     * field reaches MaxAge, it is reflooded in an attempt to flush the
     * LSA from the routing domain. LSAs of age MaxAge
     * are not used in the routing table calculation.	The value of
     * MaxAge is set to 1 hour.
     */
    static const uint32_t MaxAge = 60 * 60;

    /**
     * When the age of an LSA in the link state database hits a
     * multiple of CheckAge, the LSA's checksum is verified.  An
     * incorrect checksum at this time indicates a serious error.  The
     * value of CheckAge is set to 5 minutes.
     */
    static const uint32_t CheckAge = 5 * 60;

    /*
     * The maximum time dispersion that can occur, as an LSA is flooded
     * throughout the AS.  Most of this time is accounted for by the
     * LSAs sitting on router output queues (and therefore not aging)
     * during the flooding process.  The value of MaxAgeDiff is set to
     * 15 minutes.
     */
    static const int32_t MaxAgeDiff = 15 * 60;

    /*
     * The metric value indicating that the destination described by an
     * LSA is unreachable. Used in summary-LSAs and AS-external-LSAs as
     * an alternative to premature aging. It is
     * defined to be the 24-bit binary value of all ones: 0xffffff.
     */
    static const uint32_t LSInfinity = 0xffffff;

    /*
     * The Destination ID that indicates the default route.  This route  
     * is used when no other matching routing table entry can be found.  
     * The default destination can only be advertised in AS-external-    
     * LSAs and in stub areas' type 3 summary-LSAs.  Its value is the    
     * IP address 0.0.0.0. Its associated Network Mask is also always    
     * 0.0.0.0.                                                          
     */
    static const uint32_t DefaultDestination = 0;

    /*
     * The value used for LS Sequence Number when originating the first
     * instance of any LSA.
     */
    static const int32_t InitialSequenceNumber = 0x80000001;

    /*
     * The maximum value that LS Sequence Number can attain.
     */
    static const int32_t MaxSequenceNumber = 0x7fffffff;
};

/**
 * Interface name of a virtual link endpoint.
 */
static const char VLINK[] = "vlink";

/**
 * MTU of a virtual link.
 */
static const uint32_t VLINK_MTU = 576;

/**
 * XRL target name.
 */
static const char TARGET_OSPFv2[] = "ospfv2";
static const char TARGET_OSPFv3[] = "ospfv3";

/**
 * Get the XRL target name.
 */
inline
const char *
xrl_target(OspfTypes::Version version)
{
    switch (version) {
    case OspfTypes::V2:
	return TARGET_OSPFv2;
	break;
    case OspfTypes::V3:
	return TARGET_OSPFv3;
	break;
    }

    XLOG_UNREACHABLE();
}

/**
 * Pretty print a router or area ID.
 */
inline
string
pr_id(uint32_t id)
{
    return IPv4(htonl(id)).str();
}

/**
 * Set a router or area ID using dot notation: "128.16.64.16".
 */
inline
uint32_t
set_id(const char *addr)
{
    return ntohl(IPv4(addr).addr());
}

/**
 * Pretty print the link type.
 */
inline
string
pp_link_type(OspfTypes::LinkType link_type)
{
    switch(link_type) {
    case OspfTypes::PointToPoint:
	return "PointToPoint";
    case OspfTypes::BROADCAST:
	return "BROADCAST";
    case OspfTypes::NBMA:
	return "NBMA";
    case OspfTypes::PointToMultiPoint:
	return "PointToMultiPoint";
    case OspfTypes::VirtualLink:
	return "VirtualLink";
    }
    XLOG_UNREACHABLE();
}

/**
 * Convert from a string to the type of area
 */
inline
OspfTypes::LinkType
from_string_to_link_type(const string& type, bool& status)
{
    status = true;
    if (type == "p2p")
	return OspfTypes::PointToPoint;
    else if (type == "broadcast")
	return OspfTypes::BROADCAST;
    else if (type == "nbma")
	return OspfTypes::NBMA;
    else if (type == "p2m")
	return OspfTypes::PointToMultiPoint;
    else if (type == "vlink")
	return OspfTypes::VirtualLink;

    XLOG_WARNING("Unable to match %s", type.c_str());
    status = false;

    return OspfTypes::BROADCAST;
}

/**
 * Pretty print the area type.
 */
inline
string
pp_area_type(OspfTypes::AreaType area_type)
{
    switch(area_type) {
    case OspfTypes::NORMAL:
	return "NORMAL";
    case OspfTypes::STUB:
	return "STUB";
    case OspfTypes::NSSA:
	return "NSSA";
    }
    XLOG_UNREACHABLE();
}

/**
 * Convert from a string to the type of area
 */
inline
OspfTypes::AreaType
from_string_to_area_type(const string& type, bool& status)
{
    status = true;
    if (type == "normal")
	return OspfTypes::NORMAL;
    else if (type == "stub")
	return OspfTypes::STUB;
    else if (type == "nssa")
	return OspfTypes::NSSA;

    XLOG_WARNING("Unable to match %s", type.c_str());
    status = false;

    return OspfTypes::NORMAL;
}

/**
 * Router ID and associated interface ID. 
 */
struct RouterInfo {
    RouterInfo(OspfTypes::RouterID router_id) 
	: _router_id(router_id), _interface_id(0)
    {}

    RouterInfo(OspfTypes::RouterID router_id, uint32_t interface_id) 
	: _router_id(router_id), _interface_id(interface_id)
    {}

    OspfTypes::RouterID _router_id;	// Neighbour Router ID. 
    uint32_t _interface_id;		// Neighbour interface ID OSPFv3 only.
};

/**
 * Neighbour information that is returned by XRLs.
 */
struct NeighbourInfo {
    string _address;		// Address of neighbour.
    string _interface;		// Interface name.
    string _state;		// The current state.
    IPv4 _rid;			// The neighbours router id.
    uint32_t _priority;		// The priority in the hello packet.
    uint32_t _deadtime;		// Number of seconds before the
				// peering is considered down.
    IPv4 _area;			// The area this neighbour belongs to.
    uint32_t _opt;		// The options on the hello packet.
    IPv4 _dr;			// The designated router.
    IPv4 _bdr;			// The backup designated router.
    uint32_t _up;		// Time there has been neighbour awareness.
    uint32_t _adjacent;		// Time peering has been adjacent.
};

/**
 * OSPFv3 only, the information stored about an interface address.
 */
template <typename A>
struct AddressInfo {
    AddressInfo(A address, uint32_t prefix = 0, bool enabled = false)
	: _address(address), _prefix(prefix), _enabled(enabled)
    {}

    bool operator<(const AddressInfo<A>& other) const {
	return _address < other._address;
    }

    A _address;		// The address.
    uint32_t _prefix;	// Prefix length associated with this address.
    bool _enabled;	// True if the address should be used.
};

#include "policy_varrw.hh"
#include "io.hh"
#include "exceptions.hh"
#include "lsa.hh"
#include "packet.hh"
#include "transmit.hh"
#include "peer_manager.hh"
#include "external.hh"
#include "vlink.hh"
#include "routing_table.hh"
#include "trace.hh"

template <typename A>
class Ospf {
 public:
    Ospf(OspfTypes::Version version, EventLoop& eventloop, IO<A>* io);
	
    /**
     * @return version of OSPF this implementation represents.
     */
    OspfTypes::Version version() { return _version; }

    /**
     * @return true if ospf should still be running.
     */
    bool running() { return _io->status() != SERVICE_SHUTDOWN; }

    /**
     * Status of process.
     */
    ProcessStatus status(string& reason) {
	if (PROC_STARTUP == _process_status) {
	    if (SERVICE_RUNNING == _io->status()) {
		_process_status = PROC_READY;
		_reason = "Running";
	    }
	}

	reason = _reason;
	return _process_status;
    }

    /**
     * Shutdown OSPF.
     */
    void shutdown() {
	_io->shutdown();
	_reason = "shutting down";
	_process_status = PROC_SHUTDOWN;
    }

    /**
     * Used to send traffic on the IO interface.
     */
    bool transmit(const string& interface, const string& vif,
		  A dst, A src, uint8_t* data, uint32_t len);
    
    /**
     * The callback method that is called when data arrives on the IO
     * interface.
     */
    void receive(const string& interface, const string& vif,
		 A dst, A src, uint8_t* data, uint32_t len);

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

    /**
     * Is this interface/vif/address enabled? 
     * This is a question asked of the FEA, has the interface/vif been
     * marked as up.
     *
     * @return true if it is.
     */
    bool enabled(const string& interface, const string& vif, A address);

    /**
     * Add a callback for tracking the interface/vif status.
     *
     * The callback will be invoked whenever the status of the tuple
     * (interface, vif) is changed from disabled to enabled
     * or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_vif_status(typename IO<A>::VifStatusCb cb) {
	_io->register_vif_status(cb);
    }

    /**
     * Add a callback for tracking the interface/vif/address status.
     *
     * The callback will be invoked whenever the status of the tuple
     * (interface, vif, address) is changed from disabled to enabled
     * or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_address_status(typename IO<A>::AddressStatusCb cb) {
	_io->register_address_status(cb);
    }

    /**
     * Get all addresses associated with this interface/vif.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param addresses (out argument) list of associated addresses
     *
     * @return true if there are no errors.
     */
    bool get_addresses(const string& interface, const string& vif,
		       list<A>& addresses) const;

    /**
     * Get a link local address for this interface/vif if available.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address (out argument) set if address is found.
     *
     * @return true if a link local address is available.
     *
     */
    bool get_link_local_address(const string& interface, const string& vif,
				A& address);

    /**
     * Get the interface ID required for OSPFv3.
     * The vif argument is required for virtual links.
     */
    bool get_interface_id(const string& interface, const string& vif,
			  uint32_t& interface_id);

    /**
     * Given an interface ID return the interface and vif.
     */
    bool get_interface_vif_by_interface_id(uint32_t interface_id,
					   string& interface, string& vif);
    
    /**
     * @return prefix length for this address.
     */
    bool get_prefix_length(const string& interface, const string& vif,
			   A address, uint16_t& prefix_length);

    /**
     * @return the mtu for this interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * On the interface/vif join this multicast group.
     */
    bool join_multicast_group(const string& interface, const string& vif,
			      A mcast);
    
    /**
     * On the interface/vif leave this multicast group.
     */
    bool leave_multicast_group(const string& interface, const string& vif,
			       A mcast);

   /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t hello_interval);

#if	0
    /**
     * Set options.
     */
    bool set_options(const string& interface, const string& vif,
		     OspfTypes::AreaID area,
		     uint32_t options);
#endif

    /**
     * Create a virtual link
     *
     * @param rid neighbours router ID.
     */
    bool create_virtual_link(OspfTypes::RouterID rid);

    /**
     * Delete a virtual link
     *
     * @param rid neighbours router ID.
     */
    bool delete_virtual_link(OspfTypes::RouterID rid);

    /**
     * Attach this transit area to the neighbours router ID.
     */
    bool transit_area_virtual_link(OspfTypes::RouterID rid,
				   OspfTypes::AreaID transit_area);
    

    /**
     * Set router priority.
     */
    bool set_router_priority(const string& interface, const string& vif,
			     OspfTypes::AreaID area,
			     uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(const string& interface, const string& vif,
				  OspfTypes::AreaID area,
				  uint32_t router_dead_interval);

    /**
     * Set the interface cost.
     */
    bool set_interface_cost(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t interface_cost);

    /**
     * Set the RxmtInterval.
     */
    bool set_retransmit_interval(const string& interface, const string& vif,
				 OspfTypes::AreaID area,
				 uint16_t retransmit_interval);

    /**
     * Set InfTransDelay
     */
    bool set_inftransdelay(const string& interface, const string& vif,
			   OspfTypes::AreaID area,
			   uint16_t inftransdelay);

    /**
     * Set a simple password authentication key.
     *
     * Note that the current authentication handler is replaced with
     * a simple password authentication handler.
     *
     * @param interface the interface name.
     * @param vif the vif name.
     * @param area the area ID.
     * @param password the password to set.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_simple_authentication_key(const string& interface,
				       const string& vif,
				       OspfTypes::AreaID area,
				       const string& password,
				       string& error_msg);

    /**
     * Delete a simple password authentication key.
     *
     * Note that after the deletion the simple password authentication
     * handler is replaced with a Null authentication handler.
     *
     * @param interface the interface name.
     * @param vif the vif name.
     * @param area the area ID.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_simple_authentication_key(const string& interface,
					  const string& vif,
					  OspfTypes::AreaID area,
					  string& error_msg);

    /**
     * Set an MD5 authentication key.
     *
     * Note that the current authentication handler is replaced with
     * an MD5 authentication handler.
     *
     * @param interface the interface name.
     * @param vif the vif name.
     * @param area the area ID.
     * @param key_id unique ID associated with key.
     * @param password phrase used for MD5 digest computation.
     * @param start_timeval start time when key becomes valid.
     * @param end_timeval end time when key becomes invalid.
     * @param max_time_drift the maximum time drift among all routers.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_md5_authentication_key(const string& interface, const string& vif,
				    OspfTypes::AreaID area, uint8_t key_id,
				    const string& password,
				    const TimeVal& start_timeval,
				    const TimeVal& end_timeval,
				    const TimeVal& max_time_drift,
				    string& error_msg);

    /**
     * Delete an MD5 authentication key.
     *
     * Note that after the deletion if there are no more valid MD5 keys,
     * the MD5 authentication handler is replaced with a Null authentication
     * handler.
     *
     * @param interface the interface name.
     * @param vif the vif name.
     * @param area the area ID.
     * @param key_id the ID of the key to delete.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_md5_authentication_key(const string& interface,
				       const string& vif,
				       OspfTypes::AreaID area, uint8_t key_id,
				       string& error_msg);

    /**
     * Toggle the passive status of an interface.
     */
    bool set_passive(const string& interface, const string& vif,
		     OspfTypes::AreaID area,
		     bool passive);

    /**
     * If this is a "stub" or "nssa" area toggle the sending of a default
     *  route.
     */
    bool originate_default_route(OspfTypes::AreaID area, bool enable);

    /**
     *  Set the StubDefaultCost, the default cost sent in a default route in a
     *  "stub" or "nssa" area.
     */
    bool stub_default_cost(OspfTypes::AreaID area, uint32_t cost);

    /**
     *  Toggle the sending of summaries into "stub" or "nssa" areas.
     */
    bool summaries(OspfTypes::AreaID area, bool enable);

    /**
     * Send router alerts in IP packets or not.
     */
    bool set_ip_router_alert(bool alert);

    /**
     * Add area range.
     */
    bool area_range_add(OspfTypes::AreaID area, IPNet<A> net, bool advertise);

    /**
     * Delete area range.
     */
    bool area_range_delete(OspfTypes::AreaID area, IPNet<A> net);

    /**
     * Change the advertised state of this area.
     */
    bool area_range_change_state(OspfTypes::AreaID area, IPNet<A> net,
				 bool advertise);

    /**
     *  Get a single lsa from an area. A stateless mechanism to get LSAs. The
     *  client of this interface should start from zero and continue to request
     *  LSAs (incrementing index) until toohigh becomes true.
     *
     *  @param area database that is being searched.
     *  @param index into database starting from 0.
     *  @param valid true if a LSA has been returned. Some index values do not
     *  contain LSAs. This should not be considered an error.
     *  @param toohigh true if no more LSA exist after this index.
     *  @param self if true this LSA was originated by this router.
     *  @param lsa if valid is true the LSA at index.
     */
    bool get_lsa(const OspfTypes::AreaID area, const uint32_t index,
		 bool& valid, bool& toohigh, bool& self, vector<uint8_t>& lsa);

    /**
     *  Get a list of all the configured areas.
     */
    bool get_area_list(list<OspfTypes::AreaID>& areas) const;

    /**
     *  Get a list of all the neighbours.
     */
    bool get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const;

    /**
     * Get state information about this neighbour.
     *
     * @param nid neighbour information is being request about.
     * @param ninfo if neighbour is found its information.
     *
     */
    bool get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const;

    /**
     * Add route
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool add_route(IPNet<A> net, A nexthop, uint32_t nexthop_id,
		   uint32_t metric, bool equal, bool discard,
		   const PolicyTags& policytags);
    /**
     * Replace route
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool replace_route(IPNet<A> net, A nexthop, uint32_t nexthop_id,
		       uint32_t metric, bool equal, bool discard,
		       const PolicyTags& policytags);

    /**
     * Delete route
     */
    bool delete_route(IPNet<A> net);

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
     * Originate a route.
     *
     * @param net to announce
     * @param nexthop to forward to
     * @param metric 
     * @param policytags policy-tags associated with route.
     *
     * @return true on success
     */
    bool originate_route(const IPNet<A>& net, const A& nexthop,
			 const uint32_t& metric,
			 const PolicyTags& policytags);

    /**
     * Withdraw a route.
     *
     * @param net to withdraw
     *
     * @return true on success
     */
    bool withdraw_route(const IPNet<A>&	net);

    /**
     * Get the current OSPF version.
     */
    OspfTypes::Version get_version() const { return _version; }

    /**
     * @return a reference to the eventloop, required for timers etc...
     */
    EventLoop& get_eventloop() { return _eventloop; }

    /**
     * The test status of OSPF.
     */
    void set_testing(bool testing) {_testing = testing; }

    /**
     * @return true if OSPF is being tested.
     */
    bool get_testing() const { return _testing; }

    /**
     * @return a reference to the PeerManager.
     */
    PeerManager<A>& get_peer_manager() { return _peer_manager; }

    /**
     * @return a reference to the RoutingTable.
     */
    RoutingTable<A>& get_routing_table() { return _routing_table; }

    /**
     * @return a reference to the LSA decoder.
     */
    LsaDecoder& get_lsa_decoder() { return _lsa_decoder; }

    /**
     * @return a reference to the policy filters
     */
    PolicyFilters& get_policy_filters() { return _policy_filters; }

    /**
     * Get the Instance ID.
     */
    uint8_t get_instance_id() const { 
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _instance_id;
    }

    /**
     * Set the Instance ID.
     */
    void set_instance_id(uint8_t instance_id) { 
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_instance_id = instance_id;
    }

    /**
     * Get the Router ID.
     */
    OspfTypes::RouterID get_router_id() const { return _router_id; }

    /**
     * Set the Router ID.
     */
    void set_router_id(OspfTypes::RouterID id);

    /**
     * Get RFC 1583 compatibility.
     */
    bool get_rfc1583_compatibility() const { return _rfc1583_compatibility;}

    /**
     * Set RFC 1583 compatibility.
     */
    void set_rfc1583_compatibility(bool compatibility) {
	// Don't check for equality we can use this as a hack to force
	// routing recomputation.
	_rfc1583_compatibility = compatibility;
	_peer_manager.routing_recompute_all_areas();
    }

    Trace& trace() { return _trace; }

 private:
    const OspfTypes::Version _version;	// OSPF version.
    EventLoop& _eventloop;

    bool _testing;		// True when testing.

    IO<A>* _io;			// Indirection for sending and
				// receiving packets, as well as
				// adding and deleting routes. 
    string _reason;
    ProcessStatus _process_status;

    PacketDecoder _packet_decoder;	// Packet decoders.
    LsaDecoder _lsa_decoder;		// LSA decoders.
    PeerManager<A> _peer_manager;
    RoutingTable<A> _routing_table;
    PolicyFilters _policy_filters;	// The policy filters.

    uint8_t _instance_id;		// OSPFv3 Only

    OspfTypes::RouterID _router_id;	// Router ID.
    bool _rfc1583_compatibility;	// Preference rules for route
					// selection. 

    map<string, uint32_t> _iidmap;	// OSPFv3 only mapping of
					// interface/vif to Instance IDs.
    
    Trace _trace;		// Trace variables.
};

// The original design did not leave MaxAge LSAs in the database. When
// an LSA reached MaxAge it was removed from the database and existed
// only in retransmission lists. If an LSA was received which seemed
// to be from a previous incarnation of OSPF it had its age set to
// MaxAge and was fired out, also not being added to the database.
// If while a MaxAge LSA is on the retransmission only, either a new
// LSA such as a Network-LSA is generated or an updated LSA arrives a
// second LSA can be created with the same <Type,ID,ADV> tuple. Two LSAs
// can exist on the retransmission list. Leaving the a MaxAge LSA in
// the database solves both problems.

// #define MAX_AGE_IN_DATABASE

#define PARANOIA

#endif // __OSPF_OSPF_HH__
