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
     * Authentication type: OSPF V2 standard header.
     */
    typedef uint16_t AuType;
};

#include "io.hh"
#include "exceptions.hh"
#include "lsa.hh"
#include "packet.hh"
#include "peer_manager.hh"
#include "ls_database_manager.hh"

template <typename A>
class Ospf {
 public:
    Ospf(OspfTypes::Version version, IO* io);
	
    /**
     * @return which version of OSPF are we implementing.
     */
    OspfTypes::Version version() { return _version; }

    /**
     * @return true if ospf should still be running.
     */
    bool running() { return _running; }

    /**
     * The callback method that is called when data arrives on the IO
     * interface.
     */
    void receive(const char* interface, const char *vif,
		 const char *src,
		 const char *dst,
		 uint8_t* data, uint32_t len);

    /**
     * XXX
     * Be sure to capture the multipath capability of OSPF.
     */
    bool add_route();

    /**
     * Delete route.
     */
    bool delete_route();

    bool get_version() const { return _version; }
 private:
    const OspfTypes::Version _version;	// OSPF version.
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
