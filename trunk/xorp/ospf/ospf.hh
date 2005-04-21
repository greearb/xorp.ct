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

// $XORP$

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
    typedef IPv4 RouterID;

    /**
     * Area ID.
     */
    typedef IPv4 AreaID;

    /**
     * Link Type
     */
    enum LinkType {
	BROADCAST,
	NBMA,
	PointToMultiPoint,
	VirtualLink
    };

    /**
     * Authentication type: OSPF V2 standard header.
     */
    typedef uint16_t AuType;

    /**
     * Area Type
     */
    enum AreaType {
	BORDER,		// Area Border Router
	STUB,		// Stub Area
	NSSA,		// Not-So-Stubby Area
    };
};

/**
 * Pretty print the link type.
 */
inline
string
pp_link_type(OspfTypes::LinkType link_type)
{
    switch(link_type) {
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
#include "ls_database_manager.hh"

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
		  A dst, A src,
		  uint8_t* data, uint32_t len);

    /**
     * The callback method that is called when data arrives on the IO
     * interface.
     */
    void receive(const string& interface, const string& vif,
		 A dst, A src,
		 uint8_t* data, uint32_t len);

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

    /**
     * Set the network mask OSPFv2 only.
     */
    bool set_network_mask(const string& interface, const string& vif,
			  OspfTypes::AreaID area, 
			  uint32_t network_mask);

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

    /**
     * Set options.
     */
    bool set_options(const string& interface, const string& vif,
		     OspfTypes::AreaID area,
		     uint32_t options);

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
     * Get the Router ID.
     */
    OspfTypes::RouterID get_router_id() const { return _router_id; }

    /**
     * Set the Router ID.
     */
    void set_router_id(OspfTypes::RouterID id) { _router_id = id; }

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
    LS_database_manager<A> _database;	// Database manager.

    OspfTypes::RouterID _router_id;	// Router ID.
};

#endif // __OSPF_OSPF_HH__
