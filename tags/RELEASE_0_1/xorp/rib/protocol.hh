// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rib/protocol.hh,v 1.10 2002/12/10 06:56:08 mjh Exp $

#ifndef __RIB_PROTOCOL_HH__
#define __RIB_PROTOCOL_HH__

#include "libxorp/xorp.h"

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
     * @param proto_type either IGP or EGP
     * @param genid the generation id for the protocol (if the
     * protocol goes down and comes up, the genid should be
     * incremented).
     */
    Protocol(string name, int proto_type, int genid);

    /**
     * @return the protocol type: either IGP or EGP
     */
    int proto_type() const { return _proto_type; }

    /**
     * @return the canonical name of the routing protocol
     */
    const string& name() const { return _name; }
private:
    string _name;

    // IGP or EGP
    int _proto_type;

    int _genid;
};


/**
 * Two Protocol instances are equal if they match only in name.
 */
inline bool
operator == (Protocol a, Protocol b)
{
    return (a.name() == b.name());
}

#endif // __RIB_PROTOCOL_HH__
