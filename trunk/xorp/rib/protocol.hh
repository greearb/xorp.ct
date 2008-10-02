// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rib/protocol.hh,v 1.13 2008/07/23 05:11:30 pavlin Exp $

#ifndef __RIB_PROTOCOL_HH__
#define __RIB_PROTOCOL_HH__

#include "libxorp/xorp.h"


enum ProtocolType {
    IGP = 1,		// Interior Gateway Protocol
    EGP = 2		// Exterior Gateway Protocol
};

enum ProtocolAdminDistance {
    CONNECTED_ADMIN_DISTANCE	= 0,	// Only for connected routes.
    UNKNOWN_ADMIN_DISTANCE	= 255,
    UNSET_ADMIN_DISTANCE	= 256,
    MAX_ADMIN_DISTANCE		= 65535
};

/**
 * @short Routing protocol information.
 * 
 * Protocol holds information related to a specific routing protocol
 * that is supplying information to the RIB.  
 */
class Protocol {
public:
    /**
     * Protocol constuctor
     *
     * @param name the canonical name for the routing protocol.
     * @param protocol_type the routing protocol type (@ref ProtocolType).
     * @param genid the generation ID for the protocol (if the
     * protocol goes down and comes up, the genid should be
     * incremented).
     */
    Protocol(const string& name, ProtocolType protocol_type, uint32_t genid);

    /**
     * @return the protocol type.
     * @see ProtocolType
     */
    ProtocolType protocol_type() const { return _protocol_type; }

    /**
     * @return the canonical name of the routing protocol.
     */
    const string& name() const { return _name; }

    /**
     * Equality Operator
     * 
     * Two Protocol instances are equal if they match only in name.
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand Protocol instance is equal to
     * the right-hand protocol instance.
     */
    bool operator==(const Protocol& other) const {
	return (name() == other.name());
    }

    /**
     * Increment the protocol generation ID.
     */
    void increment_genid() { _genid++; }

private:
    string		_name;
    ProtocolType	_protocol_type;	// The protocol type (IGP or EGP)
    uint32_t		_genid;		// The protocol generation ID.
};

#endif // __RIB_PROTOCOL_HH__
