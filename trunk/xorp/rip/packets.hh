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

// $XORP: xorp/rip/packets.hh,v 1.7 2003/08/01 04:08:12 hodson Exp $

#ifndef __RIP_PACKET_ENTRIES_HH__
#define __RIP_PACKET_ENTRIES_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

#include "constants.hh"

/**
 * @short Header appearing at the start of each RIP packet.
 */
struct RipPacketHeader {
    uint8_t command;		// 1 - request, 2 - response
    uint8_t version;		// 1 - IPv4 RIPv1/IPv6 RIPng, 2 - IPv4 RIP v2
    uint8_t unused[2];

    inline void initialize(uint8_t cmd, uint8_t vers);
    inline bool valid_command() const;
    inline bool valid_version(uint8_t v) const { return version == v; }
    inline bool valid_padding() const;

    static const uint8_t REQUEST  = 1;
    static const uint8_t RESPONSE = 2;
    static const uint8_t IPv4_VERSION = 1;
    static const uint8_t IPv6_VERSION = 2;
};

inline void
RipPacketHeader::initialize(uint8_t cmd, uint8_t vers)
{
    command = cmd;
    version = vers;
    unused[0] = unused[1] = 0;
}

inline bool
RipPacketHeader::valid_command() const
{
    return command == REQUEST || command == RESPONSE;
}

inline bool
RipPacketHeader::valid_padding() const
{
    return (unused[0] | unused[1]) == 0;
}

/**
 * @short Route Entry as it appears in a RIP Packet.
 *
 * This structure is useful only it's specialized forms
 * @ref PacketRouteEntry<IPv4> and @ref PacketRouteEntry<IPv6>.
 */
template <typename A>
struct PacketRouteEntry {
};

/**
 * Smallest RIPv2 packet size.
 */
static const size_t RIPv2_MIN_PACKET_BYTES = 4;

/**
 * Smallest authenticated RIPv2 packet size.
 */
static const size_t RIPv2_MIN_AUTH_PACKET_BYTES = 4 + 20;

/**
 * Largest RIPv2 packet size.
 */
static const size_t RIPv2_MAX_PACKET_BYTES = 4 + 20 * RIPv2_ROUTES_PER_PACKET;


/**
 * @short Route Entry appearing in RIP packets on IPv4.
 *
 * This payload is carried in RIP packets on IPv4.  The entry contains
 * all the fields for RIPv2.  RIPv1 and RIPv2 use the same size
 * structure, except RIPv1 treats the route tag, subnet mask and
 * nexthop fields as Must-Be-Zero (MBZ) items.  The interpretation of
 * the fields is described in RFC2453.
 *
 * All items in the route entry are stored in network order.  The
 * accessor methods provide values in host order, and the modifiers
 * take arguments in host order.
 */
struct PacketRouteEntry<IPv4> {
protected:
    uint16_t _af;
    uint16_t _tag;
    uint32_t _addr;
    uint32_t _mask;
    uint32_t _nh;
    uint32_t _metric;

public:
    /**
     * Initialize fields as a regular routing entry.
     */
    inline void initialize(uint16_t	  tag,
			   const IPv4Net& net,
			   IPv4		  nh,
			   uint32_t	  cost);

    /**
     * Initialize fields as a route table request.
     */
    inline void     initialize_table_request();

    inline uint16_t addr_family() const 	{ return ntohs(_af); }
    inline uint16_t tag() const			{ return ntohs(_tag); }
    inline IPv4Net  net() const;
    inline IPv4     nexthop() const		{ return IPv4(_nh); }
    inline uint32_t metric() const		{ return ntohl(_metric); }

    /**
     * @return true if route entry has properties of a table request.
     */
    inline bool     is_table_request() const;

    /**
     * @return true if route entry has properties of an authentication entry.
     */
    inline bool     is_auth_entry() const;

    static const uint16_t ADDR_FAMILY = 2;
    static const uint16_t ADDR_FAMILY_DUMP = 0;
    static const uint16_t ADDR_FAMILY_AUTH = 0xffff;
};

inline void
PacketRouteEntry<IPv4>::initialize(uint16_t	  tag,
				   const IPv4Net& net,
				   IPv4		  nh,
				   uint32_t	  cost)
{
    _af     = htons(ADDR_FAMILY);
    _tag    = htons(tag);
    _addr   = net.masked_addr().addr();
    _mask   = net.netmask().addr();
    _nh     = nh.addr();
    _metric = htonl(cost);
}

inline IPv4Net
PacketRouteEntry<IPv4>::net() const
{
    return IPv4Net(IPv4(_addr), IPv4(_mask).prefix_length());
}

inline bool
PacketRouteEntry<IPv4>::is_table_request() const
{
    return addr_family() == ADDR_FAMILY_DUMP && metric() == RIP_INFINITY;
}

inline bool
PacketRouteEntry<IPv4>::is_auth_entry() const
{
    return addr_family() == ADDR_FAMILY_AUTH;
}

inline void
PacketRouteEntry<IPv4>::initialize_table_request()
{
    _af = htons(ADDR_FAMILY_DUMP);
    _tag = 0;
    _addr = 0;
    _mask = 0;
    _nh = 0;
    _metric = htonl(RIP_INFINITY);
}


/**
 * @short Route Entry for Plaintext password.
 *
 * The PlaintextPacketRouteEntry4 may appear as the first route entry
 * in a RIPv2 packet.  It has the same size as an @ref
 * PacketRouteEntry<IPv4>. The address family has the special value
 * 0xffff which imples authentication.  The authentication type is
 * overlaid in the route tag field and takes the special value 2.
 *
 * All items in the route entry are stored in network order.  The
 * accessor methods provide values in host order, and the modifiers
 * take arguments in host order.
 */
struct PlaintextPacketRouteEntry4 {
protected:
    uint16_t _af;
    uint16_t _auth;
    char     _pw[16];

public:
    inline uint16_t addr_family() const		{ return ntohs(_af); }
    inline uint16_t auth_type() const		{ return ntohs(_auth); }
    inline string   password() const		{ return string(_pw, 0, 16); }

    inline void initialize(const string& s)
    {
	_af = ADDR_FAMILY;
	_auth = htons(AUTH_TYPE);
	memset(_pw, 0, sizeof(_pw));
	s.copy(_pw, 16);
    }

    static const uint16_t ADDR_FAMILY = 0xffff;
    static const uint16_t AUTH_TYPE = 2;
};


/**
 * @short Route Entry for MD5 data.
 *
 * The MD5PacketRouteEntry4 may appear as the first route entry
 * in a RIPv2 packet.  It has the same size as an @ref
 * PacketRouteEntry<IPv4>. The address family has the special value
 * 0xffff which imples authentication.  The authentication type is
 * overlaid in the route tag field and takes the special value 3.  With
 * MD5 the authentication data appears after the remaining route entries.
 * Details are disclosed in RFC2082.
 *
 * All items in the route entry are stored in network order.  The
 * accessor methods provide values in host order, and the modifiers
 * take arguments in host order.
 *
 * NB We describe the field labelled as "RIP-2 Packet Length" on page 5 of
 * RFC 2082 as the "auth_offset".  This matches the textual description in
 * the RFC.
 */
struct MD5PacketRouteEntry4 {
protected:
    uint16_t _af;			// 0xffff - Authentication header
    uint16_t _auth;			// authentication type
    uint16_t _auth_offset;		// Offset of authentication data
    uint8_t  _key_id;			// Key number used
    uint8_t  _auth_bytes;		// auth data length at end of packet
    uint32_t _seqno;			// monotonically increasing seqno
    uint32_t _mbz[2];			// must-be-zero

public:
    inline uint16_t addr_family() const		{ return ntohs(_af); }
    inline uint16_t auth_type() const		{ return ntohs(_auth); }
    inline uint16_t auth_offset() const		{ return ntohs(_auth_offset); }
    inline uint8_t  key_id() const		{ return _key_id; }
    inline uint8_t  auth_bytes() const		{ return _auth_bytes; }
    inline uint32_t seqno() const		{ return htonl(_seqno); }

    inline void set_auth_offset(uint16_t b) 	{ _auth_offset = htons(b); }
    inline void set_key_id(uint8_t id)		{ _key_id = id; }
    inline void set_auth_bytes(uint8_t b)	{ _auth_bytes = b; }
    inline void set_seqno(uint32_t sno)		{ _seqno = htonl(sno); }
    inline void initialize(uint16_t pkt_bytes,
			   uint8_t  key_id,
			   uint8_t  auth_bytes,
			   uint32_t seqno);

    static const uint16_t ADDR_FAMILY = 0xffff;
    static const uint16_t AUTH_TYPE = 3;
};

inline void
MD5PacketRouteEntry4::initialize(uint16_t auth_offset,
				 uint8_t  key_id,
				 uint8_t  auth_bytes,
				 uint32_t seqno)
{
    _af = ADDR_FAMILY;
    _auth = htons(AUTH_TYPE);
    _mbz[0] = _mbz[1] = 0;
    set_auth_offset(auth_offset);
    set_key_id(key_id);
    set_auth_bytes(auth_bytes);
    set_seqno(seqno);
}


/**
 * @short Container for MD5 trailer.
 */
class MD5PacketTrailer {
public:
    inline void initialize();
    inline uint8_t* data() 				{ return _data; }
    inline const uint8_t* data() const			{ return _data; }
    inline uint32_t data_bytes() const			{ return 16; }
    inline bool valid() const;
protected:
    uint16_t _af;			// 0xffff - Authentication header
    uint16_t _one;			// 0x01	  - RFC2082 defined
    uint8_t  _data[16];			// 16 bytes of data
};

inline void
MD5PacketTrailer::initialize()
{
    _af = 0xffff;
    _one = htons(1);
}

bool
MD5PacketTrailer::valid() const
{
    return _af == 0xffff && htons(_one) == 1;
}


/**
 * @short Route Entry appearing in RIP packets on IPv6.
 *
 * This payload is carried in RIP packets on IPv6.  The interpretation
 * of the fields is defined in RFC2080.
 *
 * All fields in this structure are stored in network order.
 */
struct PacketRouteEntry<IPv6> {
protected:
    uint8_t  _prefix[16];
    uint16_t _tag;
    uint8_t  _prefix_length;
    uint8_t  _metric;

public:
    /**
     * Initialize fields as a route entry.
     */
    inline void	    initialize_route(uint16_t		route_tag,
				     const IPv6Net&	net,
				     uint8_t		cost);

    /**
     * Initialize fields as a nexthop entry.
     */
    inline void	    initialize_nexthop(const IPv6& nexthop);

    /**
     * Initialize fields as a route table request.
     */
    inline void	    initialize_table_request();

    inline bool	    is_nexthop() const;
    inline bool	    is_table_request() const;

    inline IPv6	    nexthop() const;

    inline uint16_t tag() const;
    inline IPv6Net  net() const;
    inline uint8_t  metric() const;

    static const uint8_t NEXTHOP_METRIC = 0xff;
};

inline void
PacketRouteEntry<IPv6>::initialize_route(uint16_t	tag,
					 const IPv6Net&	net,
					 uint8_t	cost)
{
    _tag	   = htons(tag);
    _prefix_length = net.prefix_len();
    _metric	   = cost;
    net.masked_addr().copy_out(_prefix);
}

inline void
PacketRouteEntry<IPv6>::initialize_nexthop(const IPv6& nexthop)
{
    _tag = 0;
    _prefix_length = 0;
    _metric = NEXTHOP_METRIC;
    nexthop.copy_out(_prefix);
}

inline void
PacketRouteEntry<IPv6>::initialize_table_request()
{
    memset(this, 0, sizeof(*this));
    _metric = RIP_INFINITY;
}

inline bool
PacketRouteEntry<IPv6>::is_table_request() const
{
    if (_metric != RIP_INFINITY) {
	return false;
    }
    const uint32_t* p = reinterpret_cast<const uint32_t*>(_prefix);
    return (p[0] == 0) && (p[1] == 0) && (p[2] == 0) && (p[3] == 0);
}

inline bool
PacketRouteEntry<IPv6>::is_nexthop() const
{
    return _metric == NEXTHOP_METRIC;
}

inline IPv6
PacketRouteEntry<IPv6>::nexthop() const
{
    return IPv6(_prefix);
}

inline uint16_t
PacketRouteEntry<IPv6>::tag() const
{
    return ntohs(_tag);
}

inline IPv6Net
PacketRouteEntry<IPv6>::net() const
{
    return IPv6Net(IPv6(_prefix), _prefix_length);
}

inline uint8_t
PacketRouteEntry<IPv6>::metric() const
{
    return _metric;
}


/**
 * @short RIP Packet class.
 *
 * A container for RIP packet, provides easy to use accessors and modifiers.
 */
template <typename A>
class RipPacket
{
public:
    typedef A Addr;

protected:
    Addr	    _addr;	// Src addr on inbound, dst address on outbound
    uint16_t	    _port;	// Src port on inbound, dst port on outbound
    vector<uint8_t> _data;	// Data buffer
    uint32_t _max_entries;	// Maximum number of route entries in packet

protected:
    const uint8_t* base_ptr() const	{ return &(_data[0]); }
    uint8_t* base_ptr()			{ return &(_data[0]); }

public:
    /**
     * @return destination address of packet.
     */
    inline const Addr&	address() const	{ return _addr; }

    /**
     * @return destination port of packet.
     */
    inline uint16_t	port() const	{ return _port; }

    /**
     * @return the maximum number of route entries packet may have.
     */
    inline uint32_t	max_entries() const	{ return _max_entries; }

    /**
     * Set the maximum number of route entries a packet may have.
     * This method should be called before using @ref append_data
     * methods as it resizes the internal storage and will cause
     * appended data to be lost.
     */
    inline void		set_max_entries(uint32_t max_entries);

    RipPacket(const Addr& addr, uint16_t port, uint32_t max_entries = 0)
	: _addr(addr), _port(port)
    {
	set_max_entries(max_entries);
    }

    /**
     * @return const pointer to RipPacketHeader.
     */
    inline const RipPacketHeader* header() const;

    /**
     * @return pointer to RipPacketHeader.
     */
    inline RipPacketHeader* header();

    /**
     * Route entry accessor.
     *
     * @param entry_no index of route entry to retrive
     * @return const pointer to route entry, or 0 if entry_no is greater than
     * the maximum route entries associated with packet.
     */
    inline const PacketRouteEntry<A>* route_entry(uint32_t entry_no) const;

    /**
     * Route entry accessor.
     *
     * @param entry_no index of route entry to retrive
     * @return const pointer to route entry, or 0 if entry_no is greater than
     * the maximum route entries associated with packet.
     */

    inline PacketRouteEntry<A>* route_entry(uint32_t entry_no);

    void append_data(const uint8_t* data, uint32_t data_bytes);
    void append_data(const vector<uint8_t>& data);

    inline vector<uint8_t>& 	  data()	    { return _data; }
    inline const vector<uint8_t>& data() const	    { return _data; }
    inline uint32_t 		  data_bytes() const { return _data.size(); }
    inline const uint8_t*	  data_ptr() const  { return base_ptr(); }
    inline uint8_t*	  	  data_ptr()	    { return base_ptr(); }
};

template <typename A>
const RipPacketHeader*
RipPacket<A>::header() const
{
    return reinterpret_cast<const RipPacketHeader*>(base_ptr());
}

template <typename A>
RipPacketHeader*
RipPacket<A>::header()
{
    return reinterpret_cast<RipPacketHeader*>(base_ptr());
}

template <typename A>
const PacketRouteEntry<A>*
RipPacket<A>::route_entry(uint32_t entry_no) const
{
    if (entry_no >= _max_entries)
	return 0;

    const uint8_t* p = base_ptr() + sizeof(RipPacketHeader) +
	entry_no * sizeof(PacketRouteEntry<A>);
    return reinterpret_cast<const PacketRouteEntry<A>*>(p);
}

template <typename A>
PacketRouteEntry<A>*
RipPacket<A>::route_entry(uint32_t entry_no)
{
    if (entry_no >= _max_entries)
	return 0;

    uint8_t* p = base_ptr() + sizeof(RipPacketHeader) +
	entry_no * sizeof(PacketRouteEntry<A>);
    return reinterpret_cast<PacketRouteEntry<A>*>(p);
}

template <typename A>
void
RipPacket<A>::append_data(const uint8_t* data, uint32_t data_bytes)
{
    _data.insert(_data.end(), data, data + data_bytes);
}

template <typename A>
void
RipPacket<A>::append_data(const vector<uint8_t>& data)
{
    _data.insert(_data.end(), data.begin(), data.end());
}

template <typename A>
void
RipPacket<A>::set_max_entries(uint32_t max_entries)
{
    _data.resize(sizeof(PacketRouteEntry<A>) * max_entries
		 + sizeof(RipPacketHeader));
    _max_entries = max_entries;
}

#endif // __RIP_PACKETS_HH__
