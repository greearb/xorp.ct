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

// $XORP: xorp/rip/packet_assembly.hh,v 1.3 2004/02/21 00:34:05 hodson Exp $

#ifndef __RIP_PACKET_ASSEMBLY_HH__
#define __RIP_PACKET_ASSEMBLY_HH__

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "auth.hh"
#include "packets.hh"
#include "port.hh"

/**
 * @short Internal specialized state for PacketAssembler classes.
 *
 * Completely specialized implementations exist for IPv4 and IPv6 template
 * arguments.
 */
template <typename A>
class PacketAssemblerSpecState
{};

/**
 * @short IPv4 specialized PacketAssembler state.
 *
 * This class just holder the authentication handler that IPv4 packet
 * assembly requires.
 */
template <>
class PacketAssemblerSpecState<IPv4>
{
private:
    AuthHandlerBase& _ah;
public:
    /**
     * IPv4 Specific Constructor.
     */
    PacketAssemblerSpecState(Port<IPv4>& p)
	: _ah(*(p.af_state().auth_handler()))
    {}

    /**
     * IPv4 Specific authentication handler accessor.
     */
    inline AuthHandlerBase&	  ah()			{ return _ah; }

    /**
     * IPv4 Specific authentication handler accessor.
     */
    inline const AuthHandlerBase& ah() const		{ return _ah; }
};


/**
 * @short IPv6 specialized PacketAssembler state.
 *
 * This provides a means to query the RIP port and query the
 * configured maximum entries per packet. XXX At present it's a
 * placeholder and returns a fixed value.
 *
 * It also stores the last used nexthop value since nexthops are only
 * packed when they change.
 */
template <>
class PacketAssemblerSpecState<IPv6>
{
private:
    uint32_t _max_entries;
    IPv6     _lnh;

public:
    inline PacketAssemblerSpecState(Port<IPv6>& )
	: _max_entries(25), _lnh(IPv6::ALL_ONES())
    {}

    inline uint32_t	max_entries() const;
    inline void		reset_last_nexthop();
    inline void		set_last_nexthop(const IPv6& ip6);
    inline const IPv6&	last_nexthop() const;
};

inline uint32_t
PacketAssemblerSpecState<IPv6>::max_entries() const
{
    return _max_entries;
}

inline void
PacketAssemblerSpecState<IPv6>::reset_last_nexthop()
{
    _lnh = IPv6::ALL_ONES();
}

inline void
PacketAssemblerSpecState<IPv6>::set_last_nexthop(const IPv6& ip6)
{
    _lnh = ip6;
}

inline const IPv6&
PacketAssemblerSpecState<IPv6>::last_nexthop() const
{
    return _lnh;
}


/**
 * @short Class for RIP Response Packet Assemblers.
 *
 * Both RIPv2 and RIPng have some oddities in packing and this interface
 * provides a consistent interface for that packing.
 *
 * This class has specialized IPv4 and IPv6 implementations.
 */
template <typename A>
class ResponsePacketAssembler {
public:
    typedef A				Addr;
    typedef IPNet<A>			Net;
    typedef PacketAssemblerSpecState<A> SpState;

public:
    /**
     * Constructor.
     *
     * @param p Port to take configuration information from.
     */
    inline ResponsePacketAssembler(Port<A>& p);

    /**
     * Destructor.
     */
    inline ~ResponsePacketAssembler();

    /**
     * Start assembling RIP response packet.
     */
    inline void packet_start(RipPacket<A>* pkt);

    /**
     * Add a route to RIP response packet.
     *
     * @return true if route was added, false if packet is full and would
     * have indicated this if only @ref packet_full was called.
     */
    inline bool packet_add_route(const Net&	net,
				 const Addr&	nexthop,
				 uint16_t	cost,
				 uint16_t	tag);

    /**
     * Ready-to-go accessor.
     *
     * @return true if packet has no more space for route entries.
     */
    inline bool packet_full() const;

    /**
     * Finish packet.  Some packet types require final stage processing
     * and this method gives that processing a chance to happen.  Common
     * usage is RIPv2 authentication.
     *
     * @return true on success, false if a failure is detected.
     */
    inline bool packet_finish();

private:
    /**
     * Copy Constructor (Disabled).
     */
    ResponsePacketAssembler(const ResponsePacketAssembler&);

    /**
     * Assignment Operator (Disabled).
     */
    ResponsePacketAssembler& operator=(const ResponsePacketAssembler&);

protected:
    RipPacket<A>* _p;
    uint32_t	  _pos;
    SpState	  _sp_state;
};


/**
 * @short Class to configure a RIP packet to be a table request.
 *
 * This class has specialized IPv4 and IPv6 implementations to cater for
 * address family differences.
 */
template <typename A>
class RequestTablePacketAssembler {
public:
    typedef A				Addr;
    typedef IPNet<A>			Net;
    typedef PacketAssemblerSpecState<A> SpState;

public:
    inline RequestTablePacketAssembler(Port<A>& port) : _sp_state(port) {}

    /**
     * Take RipPacket packet and make it into a table request packet.
     *
     * @return true on success, false if an error is encountered.  Should
     * an error be encountered the reason is written to the xlog facility.
     */
    inline bool prepare(RipPacket<A>* pkt);

protected:
    SpState _sp_state;
};


// ----------------------------------------------------------------------------
// ResponsePacketAssembler<IPv4> implementation

template <>
inline
ResponsePacketAssembler<IPv4>::ResponsePacketAssembler(Port<IPv4>& p)
    : _p(0), _pos(0), _sp_state(p)
{
}

template <>
inline
ResponsePacketAssembler<IPv4>::~ResponsePacketAssembler()
{
}

template <>
inline void
ResponsePacketAssembler<IPv4>::packet_start(RipPacket<IPv4>* p)
{
    _p = p;

    const AuthHandlerBase& ah = _sp_state.ah();
    _pos = ah.head_entries();
    _p->set_max_entries(ah.head_entries() + ah.max_routing_entries());
    _p->header()->initialize(RipPacketHeader::RESPONSE,
			     RipPacketHeader::IPv4_VERSION);
}

template <>
inline bool
ResponsePacketAssembler<IPv4>::packet_full() const
{
    const AuthHandlerBase& ah = _sp_state.ah();
    return _pos == ah.max_routing_entries();
}

template <>
inline bool
ResponsePacketAssembler<IPv4>::packet_add_route(const Net&	net,
						const Addr&	nexthop,
						uint16_t	cost,
						uint16_t	tag)
{
    if (packet_full()) {
	return false;
    }
    _p->route_entry(_pos)->initialize(tag, net, nexthop, cost);
    _pos++;
    return true;
}

template <>
inline bool
ResponsePacketAssembler<IPv4>::packet_finish()
{
    vector<uint8_t> trailer;

    AuthHandlerBase& ah = _sp_state.ah();

    PacketRouteEntry<IPv4>* fe = (ah.head_entries()) ? _p->route_entry(0) : 0;

    _p->set_max_entries(_pos);
    if (ah.authenticate(_p->data_ptr(), _p->data_bytes(), fe, trailer) == 0) {
	XLOG_ERROR("Outbound authentication error: %s\n", ah.error().c_str());
	return false;
    } else if (trailer.size() != 0) {
	_p->append_data(trailer);
    }
    return true;
}

// ----------------------------------------------------------------------------
// ResponsePacketAssembler<IPv6> implementation

template <>
inline
ResponsePacketAssembler<IPv6>::ResponsePacketAssembler(Port<IPv6>& p)
    : _p(0), _pos(0), _sp_state(p)
{
}

template <>
inline
ResponsePacketAssembler<IPv6>::~ResponsePacketAssembler()
{
}

template <>
inline void
ResponsePacketAssembler<IPv6>::packet_start(RipPacket<IPv6>* p)
{
    _p = p;
    _pos = 0;
    _sp_state.reset_last_nexthop();
    _p->header()->initialize(RipPacketHeader::RESPONSE,
			     RipPacketHeader::IPv6_VERSION);
}

template <>
inline bool
ResponsePacketAssembler<IPv6>::packet_full() const
{
    return (_sp_state.max_entries() - _pos) <= 2;
}

template <>
inline bool
ResponsePacketAssembler<IPv6>::packet_add_route(const Net&	net,
						const Addr&	nexthop,
						uint16_t	cost,
						uint16_t	tag)
{
    if (packet_full()) {
	return false;
    }
    if (nexthop != _sp_state.last_nexthop()) {
	_p->route_entry(_pos)->initialize_nexthop(nexthop);
	_pos++;
	_sp_state.set_last_nexthop(nexthop);
    }
    _p->route_entry(_pos)->initialize_route(tag, net, cost);
    _pos++;
    return true;
}

template <>
inline bool
ResponsePacketAssembler<IPv6>::packet_finish()
{
    _p->set_max_entries(_pos);
    return true;
}


// ----------------------------------------------------------------------------
// RequestTablePacketAssembler<IPv4> implementation

template<>
inline bool
RequestTablePacketAssembler<IPv4>::prepare(RipPacket<IPv4>* pkt)
{
    pkt->header()->initialize(RipPacketHeader::REQUEST,
			      RipPacketHeader::IPv4_VERSION);

    AuthHandlerBase& ah = _sp_state.ah();
    pkt->set_max_entries(1 + ah.head_entries());
    pkt->route_entry(ah.head_entries())->initialize_table_request();

    vector<uint8_t> trailer;
    PacketRouteEntry<IPv4>* fe = (ah.head_entries()) ? pkt->route_entry(0) : 0;

    if (ah.authenticate(pkt->data_ptr(), pkt->data_bytes(),
			fe, trailer) == 0) {
	XLOG_ERROR("Outbound authentication error: %s\n", ah.error().c_str());
	return false;
    } else if (trailer.size() != 0) {
	pkt->append_data(trailer);
    }
    return true;
}


// ----------------------------------------------------------------------------
// RequestTablePacketAssembler<IPv6> implementation

template<>
inline bool
RequestTablePacketAssembler<IPv6>::prepare(RipPacket<IPv6>* pkt)
{
    pkt->header()->initialize(RipPacketHeader::REQUEST,
			      RipPacketHeader::IPv6_VERSION);
    pkt->set_max_entries(1);
    pkt->route_entry(0)->initialize_table_request();
    return true;
}

#endif // __RIP_PACKET_ASSEMBLY_HH__
