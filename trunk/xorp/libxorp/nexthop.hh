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

// $XORP: xorp/libxorp/nexthop.hh,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $

#ifndef __LIBXORP_NEXTHOP_HH__
#define __LIBXORP_NEXTHOP_HH__

#include "xorp.h"
#include "ipv4.hh"
#include "ipv6.hh"
#include "ipvx.hh"

// NextHop is a generic next hop object.
// PeerNextHop is for next hops that are local peers.
// EncapsNextHop is for "next hops" that are non-local, and require 
//  encapsulation to reach.  Eg. PIM Register Encaps.
// ExternalNextHop An IP nexthop that is not an intermediate neighbor.
// DiscardNextHop is a discard interface.
//
// there will probably be more needed at some point

#define GENERIC_NEXTHOP		0
#define PEER_NEXTHOP		1
#define ENCAPS_NEXTHOP		2
#define EXTERNAL_NEXTHOP	3
#define DISCARD_NEXTHOP		4

/**
 * @short Generic class for next-hop information.
 *
 * NextHop is the generic class for holding information about routing
 * next hops.  NextHops can be of many types, including immediate
 * neighbors, remote routers (with IBGP), discard interfaces,
 * encapsulation endpoints, etc.  NextHop itself doesn't really do
 * anything useful, except to provide a generic handle for the
 * specialized subclasses.
 */
class NextHop {
public:
    /**
     * Default constructor
     */
    NextHop() {}

    /**
     * Destructor
     */
    virtual ~NextHop() {}

    /**
     * Get the nexthop type.
     *
     * @return the type of the nexthop.  One of:
<pre>
   GENERIC_NEXTHOP	0
   PEER_NEXTHOP		1
   ENCAPS_NEXTHOP	2
   EXTERNAL_NEXTHOP	3
   DISCARD_NEXTHOP	4
</pre>
     */
    virtual int type() = 0;

    /**
     * Convert this nexthop from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the nexthop.
     */
    virtual string str() const = 0;
};


/**
 * @short Template class for nexthop information.
 * 
 * The information contained is the nexthop address.
 */
template<class A>
class IPNextHop : public NextHop {
public:
    /**
     * Constructor from an address.
     * 
     * @param from_ipaddr @ref IPv4 or @ref IPv6 or @ref IPvX address
     * to initialize nexthop.
     */
    IPNextHop(const A &from_ipaddr);
    
    /**
     * Get the address of the nexthop.
     * 
     * @return the address of the nexthop.
     */
    inline const A& addr() { return _addr; }
    
protected:
    A _addr;
};

typedef IPNextHop<IPv4> IPv4NextHop;
typedef IPNextHop<IPv6> IPv6NextHop;
typedef IPNextHop<IPvX> IPvXNextHop;


/**
 * @short A nexthop that is an immediate neighbor.
 * 
 * Specialization of @ref IPNextHop for gateways that are the immediate
 * neighbors of this router.  Most IGP nexthops should be PeerNextHops.
 */
template<class A>
class IPPeerNextHop : public IPNextHop<A> {
public:
    /**
     * Constructor from an address.
     * 
     * @param ipv4 @ref IPv4 or @ref IPv6 or @ref IPvX address
     * to initialize nexthop.
     */
    IPPeerNextHop(const A &from_addr);
    
    /**
     * Get the type of the nexthop.
     * 
     * @return the nexthop type.  In this case, it is PEER_NEXTHOP.
     */
    int type() { return PEER_NEXTHOP; }

    /**
     * Convert this nexthop from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the nexthop.
     */
    string str() const;
    
private:
    
};

typedef IPPeerNextHop<IPv4> IPv4PeerNextHop;
typedef IPPeerNextHop<IPv6> IPv6PeerNextHop;
typedef IPPeerNextHop<IPvX> IPvXPeerNextHop;


/**
 * @short An IP nexthop that is an encapsulation tunnel.
 * 
 * Specialization of @ref IPNextHop for gateways that are encapsulation
 * tunnels.
 */
template<class A>
class IPEncapsNextHop : public IPNextHop<A> {
public:
    /**
     * Constructor from an address.
     * 
     * @param from_addr @ref IPv4 or @ref IPv6 or @ref IPvX address
     * to initialize nexthop.
     */
    IPEncapsNextHop(const A &from_addr);
    
    /**
     * Get the type of the nexthop.
     * 
     * @return the nexthop type.  In this case, it is ENCAPS_NEXTHOP.
     */
    int type() { return ENCAPS_NEXTHOP; }

    /**
     * Convert this nexthop from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the nexthop.
     */
    string str() const;
    
private:
    //_cached_peer is the cached copy of the local peer we send the 
    //encapsulated packet to.
    IPPeerNextHop<A> *_cached_peer;
};

typedef IPEncapsNextHop<IPv4> IPv4EncapsNextHop;
typedef IPEncapsNextHop<IPv6> IPv6EncapsNextHop;
typedef IPEncapsNextHop<IPvX> IPvXEncapsNextHop;


/**
 * @short An IP nexthop that is not an intermediate neighbor.
 * 
 * The nexthop that is a regular router's address, but the router
 * is not one of our immediate neighbors.
 * 
 * Specialization of @ref IPNextHop for a regular router's address, but
 * the router is not one of our immediate neighbors.  The normal case
 * when this will happen is with IBGP, where the nexthop is either the
 * exit router from the AS, or the entry router to the next AS.
 */
template<class A>
class IPExternalNextHop : public IPNextHop<A> {
public:
    /**
     * Constructor from an address.
     * 
     * @param from_addr @ref IPv4 or @ref IPv6 or @ref IPvX address
     * to initialize nexthop.
     */
    IPExternalNextHop(const A &from_addr);
    
    /**
     * Get the type of the nexthop.
     * 
     * @return the nexthop type.  In this case, it is EXTERNAL_NEXTHOP.  
     */
    int type() { return EXTERNAL_NEXTHOP; }

    /**
     * Convert this nexthop from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the nexthop.
     */
    string str() const;
    
private:
    
};

typedef IPExternalNextHop<IPv4> IPv4ExternalNextHop;
typedef IPExternalNextHop<IPv6> IPv6ExternalNextHop;
typedef IPExternalNextHop<IPvX> IPvXExternalNextHop;


/**
 * @short A nexthop that is the discard interface.
 * 
 * Specialization of @ref NextHop for blackholing traffic efficiently.
 */
class DiscardNextHop : public NextHop {
public:
    /**
     * Default constructor
     */
    DiscardNextHop();
    
    /**
     * Get the type of the nexthop.
     * 
     * @return the nexthop type.  In this case, it is DISCARD_NEXTHOP.
     */
    int type() { return DISCARD_NEXTHOP; }

    /**
     * Convert this nexthop from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the nexthop.
     */
    string str() const;
    
private:
    
};

#endif // __LIBXORP_NEXTHOP_HH__
