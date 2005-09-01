// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/ospf/ospf.hh,v 1.40 2005/08/31 23:29:14 atanu Exp $

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

    /**
     * Area Type
     */
    enum AreaType {
	BORDER,		// Border Area
	STUB,		// Stub Area
	NSSA,		// Not-So-Stubby Area
    };

    /**
     * The AreaID for the backbone area.
     */
    static const AreaID BACKBONE = 0;

    /**
     * An opaque handle that identifies a neighbour.
     */
    typedef uint32_t NeighbourID;

    /**
     * The IP protocol number used by OSPF.
     */
    static const uint16_t IP_PROTOCOL_NUMBER = 89;

    /** 
     * An identifier meaning all neighbours. No single neighbour can
     * have this identifier.
     */
    static const NeighbourID ALLNEIGHBOURS = 0;

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
     * The value used for LS Sequence Number when originating the first
     * instance of any LSA.
     */
    static const uint32_t InitialSequenceNumber = 0x80000001;

    /*
     * The maximum value that LS Sequence Number can attain.
     */
    static const uint32_t MaxSequenceNumber = 0x7fffffff;
};

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
 * Pretty print the area type.
 */
inline
string
pp_area_type(OspfTypes::AreaType area_type)
{
    switch(area_type) {
    case OspfTypes::BORDER:
	return "BORDER";
    case OspfTypes::STUB:
	return "STUB";
    case OspfTypes::NSSA:
	return "NSSA";
    }
    XLOG_UNREACHABLE();
}

#include "io.hh"
#include "exceptions.hh"
#include "lsa.hh"
#include "packet.hh"
#include "transmit.hh"
#include "peer_manager.hh"
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
    bool running() { return _running; }

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
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(const string& interface, const string& vif,
			  OspfTypes::AreaID area,
			  uint32_t interface_id);

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
			    uint16_t interaface_cost);

    /**
     * Set InfTransDelay
     */
    bool set_inftransdelay(const string& interface, const string& vif,
			   OspfTypes::AreaID area,
			   uint16_t inftransdelay);

    /**
     * XXX
     * Be sure to capture the multipath capability of OSPF.
     */
    bool add_route();

    /**
     * Delete route.
     */
    bool delete_route();

    /**
     * Get the current OSPF version.
     */
    const OspfTypes::Version get_version() const { return _version; }

    /**
     * @return a reference to the eventloop, required for timers etc...
     */
    EventLoop& get_eventloop() { return _eventloop; }

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
     * Get the Router ID.
     */
    OspfTypes::RouterID get_router_id() const { return _router_id; }

    /**
     * Set the Router ID.
     */
    void set_router_id(OspfTypes::RouterID id) { _router_id = id; }


    Trace& trace() { return _trace; }

 private:
    const OspfTypes::Version _version;	// OSPF version.
    EventLoop& _eventloop;

    IO<A>* _io;			// Indirection for sending and
				// receiving packets, as well as
				// adding and deleting routes. 
    bool _running;		// Are we running?

    PacketDecoder _packet_decoder;	// Packet decoders.
    LsaDecoder _lsa_decoder;		// LSA decoders.
    PeerManager<A> _peer_manager;
    RoutingTable<A> _routing_table;

    OspfTypes::RouterID _router_id;	// Router ID.

    Trace _trace;		// Trace variables.
};

#endif // __OSPF_OSPF_HH__
