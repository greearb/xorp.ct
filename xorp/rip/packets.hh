// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rip/packets.hh,v 1.30 2008/10/02 21:58:16 bms Exp $

#ifndef __RIP_PACKET_ENTRIES_HH__
#define __RIP_PACKET_ENTRIES_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/utility.h"

#include "libproto/packet.hh"

#include "constants.hh"

/**
 * @short Header appearing at the start of each RIP packet.
 *
 * The RIP packet header has the following content:
 *
 * command (1 byte):	// The command: 1 - request, 2 - response
 * version (1 byte):	// 1 - IPv4 RIPv1/IPv6 RIPng, 2 - IPv4 RIP v2
 * unused0 (1 byte):	// Unused
 * unused1 (1 byte):	// Unused
 */
class RipPacketHeader {
public:
    RipPacketHeader(const uint8_t* data)
	: _data(data),
	  _command(_data + _command_offset),
	  _version(_data + _version_offset),
	  _unused0(_data + _unused0_offset),
	  _unused1(_data + _unused1_offset)
    {
	static_assert(RipPacketHeader::SIZE == _command_sizeof
		      + _version_sizeof + _unused0_sizeof + _unused1_sizeof);
	static_assert(RipPacketHeader::SIZE == _unused1_offset
		      + _unused1_sizeof);
    }

    static const uint8_t REQUEST  = 1;
    static const uint8_t RESPONSE = 2;
    static const uint8_t IPv4_VERSION = RIP_AF_CONSTANTS<IPv4>::PACKET_VERSION;
    static const uint8_t IPv6_VERSION = RIP_AF_CONSTANTS<IPv6>::PACKET_VERSION;
    static const size_t SIZE = 4;	// The header size

    /**
     * Get the RIP packet header size.
     *
     * @return the RIP packet header size.
     */
    static size_t size()	{ return RipPacketHeader::SIZE; }

    uint8_t command() const	{ return extract_8(_command); }
    uint8_t version() const	{ return extract_8(_version); }
    uint8_t unused0() const	{ return extract_8(_unused0); }
    uint8_t unused1() const	{ return extract_8(_unused1); }

    bool valid_command() const;
    bool valid_version(uint8_t v) const { return version() == v; }
    bool valid_padding() const;

protected:
    // Sizes of the fields
    static const size_t _command_sizeof = 1;
    static const size_t _version_sizeof = 1;
    static const size_t _unused0_sizeof = 1;
    static const size_t _unused1_sizeof = 1;

    // Offsets for the fields
    static const size_t _command_offset = 0;
    static const size_t _version_offset = _command_offset + _command_sizeof;
    static const size_t _unused0_offset = _version_offset + _version_sizeof;
    static const size_t _unused1_offset = _unused0_offset + _unused0_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _command;	// 1 - request, 2 - response
    const uint8_t* _version;	// 1 - IPv4 RIPv1/IPv6 RIPng, 2 - IPv4 RIP v2
    const uint8_t* _unused0;
    const uint8_t* _unused1;
};

inline bool
RipPacketHeader::valid_command() const
{
    return command() == REQUEST || command() == RESPONSE;
}

inline bool
RipPacketHeader::valid_padding() const
{
    return (unused0() | unused1()) == 0;
}

/*
 * @short Class for writing data to RIP packet header.
 */
class RipPacketHeaderWriter : public RipPacketHeader {
public:
    RipPacketHeaderWriter(uint8_t* data)
	: RipPacketHeader(data),
	  _data(data),
	  _command(_data + _command_offset),
	  _version(_data + _version_offset),
	  _unused0(_data + _unused0_offset),
	  _unused1(_data + _unused1_offset)
    {}

    void initialize(uint8_t cmd, uint8_t vers);

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _command;		// 1 - request, 2 - response
    uint8_t* _version;		// 1 - IPv4 RIPv1/IPv6 RIPng, 2 - IPv4 RIP v2
    uint8_t* _unused0;
    uint8_t* _unused1;
};

inline void
RipPacketHeaderWriter::initialize(uint8_t cmd, uint8_t vers)
{
    embed_8(_command, cmd);
    embed_8(_version, vers);
    embed_8(_unused0, 0);
    embed_8(_unused1, 0);
}

/**
 * @short Route Entry as it appears in a RIP Packet.
 *
 * This structure is useful only it's specialized forms
 * @ref PacketRouteEntry<IPv4> and @ref PacketRouteEntry<IPv6>.
 */
template <typename A>
class PacketRouteEntry {
};

/*
 * @short Class for writing data to RIP route entry.
 *
 * This structure is useful only it's specialized forms
 * @ref PacketRouteEntryWriter<IPv4> and @ref PacketRouteEntryWriter<IPv6>.
 */
template <typename A>
class PacketRouteEntryWriter {
};

/**
 * Smallest RIPv2 packet size.
 */
static const size_t RIPv2_MIN_PACKET_BYTES = 4;
static const size_t RIP_MIN_PACKET_BYTES = 4;

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
 *
 * The route entry has the following content:
 *
 * af (2 bytes):	// The address family
 * tag (2 bytes):	// Tag
 * addr (4 bytes):	// Address
 * mask (4 bytes):	// Mask
 * nh (4 bytes):	// Next hop
 * metric (4 bytes):	// Metric
 */
template <>
class PacketRouteEntry<IPv4> {
public:
    PacketRouteEntry(const uint8_t* data)
	: _data(data),
	  _af(_data + _af_offset),
	  _tag(_data + _tag_offset),
	  _addr(_data + _addr_offset),
	  _mask(_data + _mask_offset),
	  _nh(_data + _nh_offset),
	  _metric(_data + _metric_offset)
    {
	static_assert(PacketRouteEntry<IPv4>::SIZE == _af_sizeof + _tag_sizeof
		      + _addr_sizeof + _mask_sizeof + _nh_sizeof
		      + _metric_sizeof);
	static_assert(PacketRouteEntry<IPv4>::SIZE == _metric_offset
		      + _metric_sizeof);
    }

    static const size_t SIZE = 20;	// The entry size

    /**
     * Get the RIP IPv4 route entry size.
     *
     * @return the RIP IPv4 route entry size.
     */
    static size_t size()	{ return PacketRouteEntry<IPv4>::SIZE; }

    uint16_t	addr_family() const 	{ return extract_16(_af); }
    uint16_t	tag() const		{ return extract_16(_tag); }
    IPv4	addr() const		{ return IPv4(_addr); }
    uint32_t	prefix_len() const;
    IPv4Net	net() const;
    IPv4	nexthop() const		{ return IPv4(_nh); }
    uint32_t	metric() const		{ return extract_32(_metric); }

    /**
     * @return true if route entry has properties of a table request.
     */
    bool	is_table_request() const;

    /**
     * @return true if route entry has properties of an authentication entry.
     */
    bool	is_auth_entry() const;

    static const uint16_t ADDR_FAMILY = 2;
    static const uint16_t ADDR_FAMILY_DUMP = 0;
    static const uint16_t ADDR_FAMILY_AUTH = 0xffff;

protected:
    // Sizes of the fields
    static const size_t _af_sizeof	= 2;
    static const size_t _tag_sizeof	= 2;
    static const size_t _addr_sizeof	= 4;
    static const size_t _mask_sizeof	= 4;
    static const size_t _nh_sizeof	= 4;
    static const size_t _metric_sizeof	= 4;

    // Offsets for the fields
    static const size_t _af_offset	= 0;
    static const size_t _tag_offset	= _af_offset + _af_sizeof;
    static const size_t _addr_offset	= _tag_offset + _tag_sizeof;
    static const size_t _mask_offset	= _addr_offset + _addr_sizeof;
    static const size_t _nh_offset	= _mask_offset + _mask_sizeof;
    static const size_t _metric_offset	= _nh_offset + _nh_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _af;
    const uint8_t* _tag;
    const uint8_t* _addr;
    const uint8_t* _mask;
    const uint8_t* _nh;
    const uint8_t* _metric;
};

inline uint32_t
PacketRouteEntry<IPv4>::prefix_len() const
{
    return IPv4(_mask).mask_len();
}

inline IPv4Net
PacketRouteEntry<IPv4>::net() const
{
    return IPv4Net(IPv4(_addr), IPv4(_mask).mask_len());
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

/*
 * @short Class for writing data to RIP IPv4 route entry.
 */
template <>
class PacketRouteEntryWriter<IPv4> : public PacketRouteEntry<IPv4> {
public:
    PacketRouteEntryWriter(uint8_t* data)
	: PacketRouteEntry<IPv4>(data),
	  _data(data),
	  _af(_data + _af_offset),
	  _tag(_data + _tag_offset),
	  _addr(_data + _addr_offset),
	  _mask(_data + _mask_offset),
	  _nh(_data + _nh_offset),
	  _metric(_data + _metric_offset)
    {}

    /**
     * Initialize fields as a regular routing entry.
     */
    void initialize(uint16_t		tag,
		    const IPv4Net&	net,
		    IPv4		nh,
		    uint32_t		cost);

    /**
     * Initialize fields as a route table request.
     */
    void initialize_table_request();

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _af;
    uint8_t* _tag;
    uint8_t* _addr;
    uint8_t* _mask;
    uint8_t* _nh;
    uint8_t* _metric;
};

inline void
PacketRouteEntryWriter<IPv4>::initialize(uint16_t	tag,
					 const IPv4Net&	net,
					 IPv4		nh,
					 uint32_t	cost)
{
    embed_16(_af, ADDR_FAMILY);
    embed_16(_tag, tag);
    net.masked_addr().copy_out(_addr);
    net.netmask().copy_out(_mask);
    nh.copy_out(_nh);
    embed_32(_metric, cost);
}

inline void
PacketRouteEntryWriter<IPv4>::initialize_table_request()
{
    embed_16(_af, ADDR_FAMILY_DUMP);
    embed_16(_tag, 0);
    IPv4::ZERO().copy_out(_addr);
    IPv4::ZERO().copy_out(_mask);
    IPv4::ZERO().copy_out(_nh);
    embed_32(_metric, RIP_INFINITY);
}


/**
 * @short Route Entry for Plaintext password.
 *
 * The PlaintextPacketRouteEntry4 may appear as the first route entry
 * in a RIPv2 packet.  It has the same size as an @ref
 * PacketRouteEntry<IPv4>. The address family has the special value
 * 0xffff which implies authentication.  The authentication type is
 * overlaid in the route tag field and takes the special value 2.
 *
 * All items in the route entry are stored in network order.  The
 * accessor methods provide values in host order, and the modifiers
 * take arguments in host order.
 *
 * The plaintext authentication entry has the following content:
 *
 * af (2 bytes):	// The address family
 * auth (2 bytes):	// Authentication type
 * pw (16 bytes):	// Password
 */
class PlaintextPacketRouteEntry4 {
public:
    PlaintextPacketRouteEntry4(const uint8_t* data)
	: _data(data),
	  _af(_data + _af_offset),
	  _auth(_data + _auth_offset),
	  _pw(_data + _pw_offset)
    {
	static_assert(PlaintextPacketRouteEntry4::SIZE == _af_sizeof
		      + _auth_sizeof + _pw_sizeof);
	static_assert(PlaintextPacketRouteEntry4::SIZE == _pw_offset 
		      + _pw_sizeof);
    }

    static const size_t SIZE = 20;	// The entry size

    /**
     * Get the RIP IPv4 plaintext password route entry size.
     *
     * @return the RIP IPv4 plaintext password route entry size.
     */
    static size_t size()	{ return PlaintextPacketRouteEntry4::SIZE; }

    uint16_t addr_family() const	{ return extract_16(_af); }
    uint16_t auth_type() const		{ return extract_16(_auth); }
    string   password() const;

    static const uint16_t ADDR_FAMILY = 0xffff;
    static const uint16_t AUTH_TYPE = 2;

protected:
    // Sizes of the fields
    static const size_t _af_sizeof	= 2;
    static const size_t _auth_sizeof	= 2;
    static const size_t _pw_sizeof	= 16;

    // Offsets for the fields
    static const size_t _af_offset	= 0;
    static const size_t _auth_offset	= _af_offset + _af_sizeof;
    static const size_t _pw_offset	= _auth_offset + _auth_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _af;
    const uint8_t* _auth;
    const uint8_t* _pw;
};

inline string
PlaintextPacketRouteEntry4::password() const
{
    return string(reinterpret_cast<const char *>(_pw), 0, _pw_sizeof);
}

/**
 * @short Class for writing data to Plaintext password authentication entry.
 */
class PlaintextPacketRouteEntry4Writer : public PlaintextPacketRouteEntry4 {
public:
    PlaintextPacketRouteEntry4Writer(uint8_t* data)
	: PlaintextPacketRouteEntry4(data),
	  _data(data),
	  _af(_data + _af_offset),
	  _auth(_data + _auth_offset),
	  _pw(_data + _pw_offset)
    {}

    /**
     * Initialize the entry.
     */
    void initialize(const string& s);

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _af;
    uint8_t* _auth;
    uint8_t* _pw;
};

inline void
PlaintextPacketRouteEntry4Writer::initialize(const string& s)
{
    embed_16(_af, ADDR_FAMILY);
    embed_16(_auth, AUTH_TYPE);
    memset(_pw, 0, _pw_sizeof);
    s.copy(reinterpret_cast<char *>(_pw), _pw_sizeof);
}


/**
 * @short Route Entry for MD5 data.
 *
 * The MD5PacketRouteEntry4 may appear as the first route entry
 * in a RIPv2 packet.  It has the same size as an @ref
 * PacketRouteEntry<IPv4>. The address family has the special value
 * 0xffff which implies authentication.  The authentication type is
 * overlaid in the route tag field and takes the special value 3.  With
 * MD5 the authentication data appears after the remaining route entries.
 * Details are disclosed in RFC2082.
 *
 * All items in the route entry are stored in network order.  The
 * accessor methods provide values in host order, and the modifiers
 * take arguments in host order.
 *
 * NB We describe the field labelled as "RIP-2 Packet Length" on page 5 of
 * RFC 2082 as the "auth_off".  This matches the textual description in
 * the RFC.
 *
 * The MD5 authentication entry has the following content:
 *
 * af (2 bytes):		// 0xffff - Authentication header
 * auth (2 bytes):		// Authentication type
 * auth_off (2 bytes):		// Offset of authentication data
 * key_id (1 byte):		// Key number used
 * auth_bytes (1 byte):		// Auth data length at end of packet
 * seqno (4 bytes):		// Monotonically increasing seqno
 * mbz (8 bytes):		// Must-be-zero
 */
class MD5PacketRouteEntry4 {
public:
    MD5PacketRouteEntry4(const uint8_t* data)
	: _data(data),
	  _af(_data + _af_offset),
	  _auth(_data + _auth_offset),
	  _auth_off(_data + _auth_off_offset),
	  _key_id(_data + _key_id_offset),
	  _auth_bytes(_data + _auth_bytes_offset),
	  _seqno(_data + _seqno_offset),
	  _mbz(_data + _mbz_offset)
    {
	static_assert(MD5PacketRouteEntry4::SIZE == _af_sizeof + _auth_sizeof
		      + _auth_off_sizeof + _key_id_sizeof + _auth_bytes_sizeof
		      + _seqno_sizeof + _mbz_sizeof);
	static_assert(MD5PacketRouteEntry4::SIZE == _mbz_offset + _mbz_sizeof);
    }

    static const size_t SIZE = 20;	// The entry size

    /**
     * Get the RIP IPv4 MD5 authentication route entry size.
     *
     * @return the RIP IPv4 MD5 authentication route entry size.
     */
    static size_t size()		{ return MD5PacketRouteEntry4::SIZE; }

    uint16_t addr_family() const	{ return extract_16(_af); }
    uint16_t auth_type() const		{ return extract_16(_auth); }
    uint16_t auth_off() const		{ return extract_16(_auth_off); }
    uint8_t  key_id() const		{ return extract_8(_key_id); }
    uint8_t  auth_bytes() const		{ return extract_8(_auth_bytes); }
    uint32_t seqno() const		{ return extract_32(_seqno); }

    static const uint16_t ADDR_FAMILY = 0xffff;
    static const uint16_t AUTH_TYPE = 3;

protected:
    // Sizes of the fields
    static const size_t _af_sizeof		= 2;
    static const size_t _auth_sizeof		= 2;
    static const size_t _auth_off_sizeof	= 2;
    static const size_t _key_id_sizeof		= 1;
    static const size_t _auth_bytes_sizeof	= 1;
    static const size_t _seqno_sizeof		= 4;
    static const size_t _mbz_sizeof		= 8;

    // Offsets for the fields
    static const size_t _af_offset	= 0;
    static const size_t _auth_offset	= _af_offset + _af_sizeof;
    static const size_t _auth_off_offset = _auth_offset + _auth_sizeof;
    static const size_t _key_id_offset	= _auth_off_offset + _auth_off_sizeof;
    static const size_t _auth_bytes_offset = _key_id_offset + _key_id_sizeof;
    static const size_t _seqno_offset = _auth_bytes_offset + _auth_bytes_sizeof;
    static const size_t _mbz_offset	= _seqno_offset + _seqno_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _af;		// 0xffff - Authentication header
    const uint8_t* _auth;	// Authentication type
    const uint8_t* _auth_off;	// Offset of authentication data
    const uint8_t* _key_id;	// Key number used
    const uint8_t* _auth_bytes;	// Auth data length at end of packet
    const uint8_t* _seqno;	// Monotonically increasing seqno
    const uint8_t* _mbz;	// Must-be-zero
};

/**
 * @short Class for writing data to MD5 authentication entry.
 */
class MD5PacketRouteEntry4Writer : public MD5PacketRouteEntry4 {
public:
    MD5PacketRouteEntry4Writer(uint8_t* data)
	: MD5PacketRouteEntry4(data),
	  _data(data),
	  _af(_data + _af_offset),
	  _auth(_data + _auth_offset),
	  _auth_off(_data + _auth_off_offset),
	  _key_id(_data + _key_id_offset),
	  _auth_bytes(_data + _auth_bytes_offset),
	  _seqno(_data + _seqno_offset),
	  _mbz(_data + _mbz_offset)
    {}

    void set_auth_off(uint16_t b) 	{ embed_16(_auth_off, b); }
    void set_key_id(uint8_t id)		{ embed_8(_key_id, id); }
    void set_auth_bytes(uint8_t b)	{ embed_8(_auth_bytes, b); }
    void set_seqno(uint32_t sno)	{ embed_32(_seqno, sno); }

    /**
     * Initialize the entry.
     */
    void initialize(uint16_t pkt_bytes,
		    uint8_t  key_id,
		    uint8_t  auth_bytes,
		    uint32_t seqno);

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _af;		// 0xffff - Authentication header
    uint8_t* _auth;		// Authentication type
    uint8_t* _auth_off;		// Offset of authentication data
    uint8_t* _key_id;		// Key number used
    uint8_t* _auth_bytes;	// Auth data length at end of packet
    uint8_t* _seqno;		// Monotonically increasing seqno
    uint8_t* _mbz;		// Must-be-zero
};

inline void
MD5PacketRouteEntry4Writer::initialize(uint16_t auth_off,
				       uint8_t  key_id,
				       uint8_t  auth_bytes,
				       uint32_t seqno)
{
    embed_16(_af, ADDR_FAMILY);
    embed_16(_auth, AUTH_TYPE);
    set_auth_off(auth_off);
    set_key_id(key_id);
    set_auth_bytes(auth_bytes);
    set_seqno(seqno);
    memset(_mbz, 0, _mbz_sizeof);
}


/**
 * @short Container for MD5 trailer.
 *
 * THE MD5 authentication trailer has the following content:
 *
 * af (2 bytes):		// 0xffff - Authentication header
 * one (2 bytes):		// 0x01	  - RFC2082 defined
 * auth_data (16 bytes);	// 16 bytes of data
 */
class MD5PacketTrailer {
public:
    MD5PacketTrailer(const uint8_t* data)
	: _data(data),
	  _af(_data + _af_offset),
	  _one(_data + _one_offset),
	  _auth_data(_data + _auth_data_offset)
    {
	static_assert(MD5PacketTrailer::SIZE == _af_sizeof + _one_sizeof
		      + _auth_data_sizeof);
	static_assert(MD5PacketTrailer::SIZE == _auth_data_offset
		      + _auth_data_sizeof);
    }

    static const size_t SIZE = 20;	// The trailer size

    /**
     * Get the RIP IPv4 MD5 authentication trailer size.
     *
     * @return the RIP IPv4 MD5 authentication trailer size.
     */
    static size_t size()		{ return MD5PacketTrailer::SIZE; }

    uint16_t addr_family() const	{ return extract_16(_af); }
    const uint8_t* auth_data() const	{ return _auth_data; }
    uint32_t auth_data_bytes() const	{ return _auth_data_sizeof; }
    uint32_t auth_data_offset() const	{ return _auth_data_offset; }
    bool valid() const;

protected:
    // Sizes of the fields
    static const size_t _af_sizeof		= 2;
    static const size_t _one_sizeof		= 2;
    static const size_t _auth_data_sizeof	= 16;

    // Offsets for the fields
    static const size_t _af_offset		= 0;
    static const size_t _one_offset		= _af_offset + _af_sizeof;
    static const size_t _auth_data_offset	= _one_offset + _one_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _af;		// 0xffff - Authentication header
    const uint8_t* _one;	// 0x01	  - RFC2082 defined
    const uint8_t* _auth_data;	// 16 bytes of data
};

inline bool
MD5PacketTrailer::valid() const
{
    return (addr_family() == 0xffff) && (extract_16(_one) == 1);
}

/**
 * @short Class for writing data to MD5 authentication trailer.
 */
class MD5PacketTrailerWriter : public MD5PacketTrailer {
public:
    MD5PacketTrailerWriter(uint8_t* data)
	: MD5PacketTrailer(data),
	  _data(data),
	  _af(_data + _af_offset),
	  _one(_data + _one_offset),
	  _auth_data(_data + _auth_data_offset)
    {}

    /**
     * Initialize the entry.
     */
    void initialize();

    uint8_t* auth_data()	{ return _auth_data; }

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _af;		// 0xffff - Authentication header
    uint8_t* _one;		// 0x01	  - RFC2082 defined
    uint8_t* _auth_data;	// 16 bytes of data
};

inline void
MD5PacketTrailerWriter::initialize()
{
    embed_16(_af, 0xffff);
    embed_16(_one, 1);
}


/**
 * @short Route Entry appearing in RIP packets on IPv6.
 *
 * This payload is carried in RIP packets on IPv6.  The interpretation
 * of the fields is defined in RFC2080.
 *
 * All fields in this structure are stored in network order.
 *
 * The route entry has the following content:
 *
 * prefix (16 bytes):	// Prefix
 * tag (2 bytes):	// Tag
 * prefix_len (1 byte):	// Prefix length
 * metric (1 byte):	// Metric
 */
template <>
class PacketRouteEntry<IPv6> {
public:
    PacketRouteEntry(const uint8_t* data)
	: _data(data),
	  _prefix(_data + _prefix_offset),
	  _tag(_data + _tag_offset),
	  _prefix_len(_data + _prefix_len_offset),
	  _metric(_data + _metric_offset)
    {
	static_assert(PacketRouteEntry<IPv6>::SIZE == _prefix_sizeof
		      + _tag_sizeof + _prefix_len_sizeof + _metric_sizeof);
	static_assert(PacketRouteEntry<IPv6>::SIZE == _metric_offset
		      + _metric_sizeof);
    }

    static const size_t SIZE = 20;	// The entry size

    /**
     * Get the RIP IPv6 route entry size.
     *
     * @return the RIP IPv6 route entry size.
     */
    static size_t size()	{ return PacketRouteEntry<IPv6>::SIZE; }

    bool	    is_nexthop() const;

    /**
     * @return true if route entry has properties of a table request.
     */
    bool	    is_table_request() const;

    IPv6	    nexthop() const;

    uint16_t tag() const;
    uint32_t prefix_len() const { return extract_8(_prefix_len); }
    IPv6Net  net() const;
    uint8_t  metric() const;

    static const uint8_t NEXTHOP_METRIC = 0xff;


protected:
    // Sizes of the fields
    static const size_t _prefix_sizeof		= 16;
    static const size_t _tag_sizeof		= 2;
    static const size_t _prefix_len_sizeof	= 1;
    static const size_t _metric_sizeof		= 1;

    // Offsets for the fields
    static const size_t _prefix_offset	= 0;
    static const size_t _tag_offset	= _prefix_offset + _prefix_sizeof;
    static const size_t _prefix_len_offset = _tag_offset + _tag_sizeof;
    static const size_t _metric_offset	= _prefix_len_offset + _prefix_len_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _prefix;
    const uint8_t* _tag;
    const uint8_t* _prefix_len;
    const uint8_t* _metric;
};

inline bool
PacketRouteEntry<IPv6>::is_nexthop() const
{
    return metric() == NEXTHOP_METRIC;
}

inline IPv6
PacketRouteEntry<IPv6>::nexthop() const
{
    return IPv6(_prefix);
}

inline uint16_t
PacketRouteEntry<IPv6>::tag() const
{
    return extract_16(_tag);
}

inline IPv6Net
PacketRouteEntry<IPv6>::net() const
{
    return IPv6Net(IPv6(_prefix), prefix_len());
}

inline uint8_t
PacketRouteEntry<IPv6>::metric() const
{
    return extract_8(_metric);
}

inline bool
PacketRouteEntry<IPv6>::is_table_request() const
{
    if (metric() != RIP_INFINITY) {
	return false;
    }
    if (prefix_len() != 0) {
	return false;
    }
    IPv6 addr(_prefix);
    return (addr.is_zero());
}

/*
 * @short Class for writing data to RIP IPv6 route entry.
 */
template <>
class PacketRouteEntryWriter<IPv6> : public PacketRouteEntry<IPv6> {
public:
    PacketRouteEntryWriter(uint8_t* data)
	: PacketRouteEntry<IPv6>(data),
	  _data(data),
	  _prefix(_data + _prefix_offset),
	  _tag(_data + _tag_offset),
	  _prefix_len(_data + _prefix_len_offset),
	  _metric(_data + _metric_offset)
    {}

    /**
     * Initialize fields as a regular routing entry.
     */
    void	    initialize_route(uint16_t		route_tag,
				     const IPv6Net&	net,
				     uint8_t		cost);

    /**
     * Initialize fields as a nexthop entry.
     */
    void	    initialize_nexthop(const IPv6& nexthop);

    /**
     * Initialize fields as a route table request.
     */
    void	    initialize_table_request();

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _prefix;
    uint8_t* _tag;
    uint8_t* _prefix_len;
    uint8_t* _metric;
};

inline void
PacketRouteEntryWriter<IPv6>::initialize_route(uint16_t		tag,
					       const IPv6Net&	net,
					       uint8_t		cost)
{
    net.masked_addr().copy_out(_prefix);
    embed_16(_tag, tag);
    embed_8(_prefix_len, net.prefix_len());
    embed_8(_metric, cost);
}

inline void
PacketRouteEntryWriter<IPv6>::initialize_nexthop(const IPv6& nexthop)
{
    nexthop.copy_out(_prefix);
    embed_16(_tag, 0);
    embed_8(_prefix_len, 0);
    embed_8(_metric, NEXTHOP_METRIC);
}

inline void
PacketRouteEntryWriter<IPv6>::initialize_table_request()
{
    IPv6::ZERO().copy_out(_prefix);
    embed_16(_tag, 0);
    embed_8(_prefix_len, 0);
    embed_8(_metric, RIP_INFINITY);
}


/**
 * @short RIP Packet class.
 *
 * A container for RIP packet, provides easy to use accessors and modifiers.
 */
template <typename A>
class RipPacket {
public:
    typedef A Addr;

    /**
     * @return destination address of packet.
     */
    const Addr&	address() const	{ return _addr; }

    /**
     * @return destination port of packet.
     */
    uint16_t	port() const	{ return _port; }

    /**
     * @return the maximum number of route entries packet may have.
     */
    uint32_t	max_entries() const	{ return _max_entries; }

    /**
     * Set the maximum number of route entries a packet may have.
     * This method should be called before using @ref append_data
     * methods as it resizes the internal storage and will cause
     * appended data to be lost.
     */
    void	set_max_entries(uint32_t max_entries);

    RipPacket(const Addr& addr,
	      uint16_t	  port,
	      uint32_t	  max_entries = RIPv2_ROUTES_PER_PACKET)
	: _addr(addr), _port(port), _max_entries(0)
    {
	set_max_entries(max_entries);
    }

    /**
     * @return const pointer to the beginning of the RIP packet header.
     */
    const uint8_t* header_ptr() const;

    /**
     * @return pointer to the beginning of the RIP packet header.
     */
    uint8_t* header_ptr();

    /**
     * Route entry accessor.
     *
     * @param entry_no index of route entry to retrive.
     * @return const pointer to route entry, or 0 if entry_no is greater than
     * the maximum route entries associated with packet.
     */
    const uint8_t* route_entry_ptr(uint32_t entry_no) const;

    /**
     * Route entry accessor.
     *
     * @param entry_no index of route entry to retrive.
     * @return pointer to route entry, or 0 if entry_no is greater than
     * the maximum route entries associated with packet.
     */
    uint8_t* route_entry_ptr(uint32_t entry_no);

    void append_data(const uint8_t* data, uint32_t data_bytes);
    void append_data(const vector<uint8_t>& data);

    vector<uint8_t>& 	  data()		{ return _data; }
    const vector<uint8_t>& data() const		{ return _data; }
    uint32_t 		  data_bytes() const	{ return _data.size(); }
    const uint8_t*	  data_ptr() const	{ return base_ptr(); }
    uint8_t*	  	  data_ptr()		{ return base_ptr(); }

private:
    Addr	    _addr;	// Src addr on inbound, dst address on outbound
    uint16_t	    _port;	// Src port on inbound, dst port on outbound
    vector<uint8_t> _data;	// Data buffer
    uint32_t _max_entries;	// Maximum number of route entries in packet

    const uint8_t* base_ptr() const	{ return &(_data[0]); }
    uint8_t* base_ptr()			{ return &(_data[0]); }
};

template <typename A>
const uint8_t*
RipPacket<A>::header_ptr() const
{
    return (base_ptr());
}

template <typename A>
uint8_t*
RipPacket<A>::header_ptr()
{
    return (base_ptr());
}

template <typename A>
const uint8_t*
RipPacket<A>::route_entry_ptr(uint32_t entry_no) const
{
    if (entry_no >= _max_entries)
	return NULL;

    const uint8_t* p = base_ptr() + RipPacketHeader::size() +
	entry_no * PacketRouteEntry<A>::size();
    return p;
}

template <typename A>
uint8_t*
RipPacket<A>::route_entry_ptr(uint32_t entry_no)
{
    if (entry_no >= _max_entries)
	return NULL;

    uint8_t* p = base_ptr() + RipPacketHeader::size() +
	entry_no * PacketRouteEntry<A>::size();
    return p;
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
    if (max_entries != _max_entries) {
	_data.resize(PacketRouteEntry<A>::size() * max_entries
		     + RipPacketHeader::size());
	_max_entries = max_entries;
    }
}

#endif // __RIP_PACKETS_HH__
