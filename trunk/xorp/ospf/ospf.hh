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
    Ospf(OspfTypes::Version version, EventLoop& eventloop, IO* io);
	
    /**
     * @return which version of OSPF are we implementing.
     */
    OspfTypes::Version version() { return _version; }

    /**
     * @return true if ospf should still be running.
     */
    bool running() { return _running; }

    /**
     * Used to send traffic on the IO interface.
     */
    bool send(const string& interface, const string& vif,
	      uint8_t* data, uint32_t len);

    /**
     * The callback method that is called when data arrives on the IO
     * interface.
     */
    void receive(const string& interface, const string& vif,
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
    bool get_version() const { return _version; }

    /**
     * @return a reference to the eventloop, required for timers etc...
     */
    EventLoop& get_eventloop() { return _eventloop; }
 private:
    const OspfTypes::Version _version;	// OSPF version.
    EventLoop& _eventloop;

    IO* _io;			// Indirection for sending and
				// receiving packets, as well as
				// adding and deleting routes. 
    bool _running;		// Are we running?

    PacketDecoder _packet_decoder;	// Packet decoders.
    LsaDecoder _lsa_decoder;		// LSA decoders.
    PeerManager<A> _peer_manager;
    LS_database_manager<A> _database;	// Database manager.
};

#endif // __OSPF_OSPF_HH__
