// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libproto/packet.hh,v 1.18 2008/10/09 18:04:11 abittau Exp $


#ifndef __LIBPROTO_PACKET_HH__
#define __LIBPROTO_PACKET_HH__


#include "libxorp/xorp.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/mac.hh"


/**
 * Extract an 8-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return an 8-bit number from the beginning of a buffer.
 */
inline
uint8_t
extract_8(const uint8_t *ptr)
{
    uint8_t val;

    val = ptr[0];

    return val;
}

/**
 * Embed an 8-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 8-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_8(uint8_t *ptr, uint8_t val)
{
    ptr[0] = val;
}

/**
 * Extract a 16-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 16-bit number from the beginning of a buffer.
 */
inline
uint16_t
extract_16(const uint8_t *ptr)
{
    uint16_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];

    return val;
}

/**
 * Embed a 16-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 16-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_16(uint8_t *ptr, uint16_t val)
{
    ptr[0] = (val >> 8) & 0xff;
    ptr[1] = val & 0xff;
}

/**
 * Extract a 24-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 24-bit number from the beginning of a buffer.
 */
inline
uint32_t
extract_24(const uint8_t *ptr)
{
    uint32_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];
    val <<= 8;
    val |= ptr[2];

    return val;
}

/**
 * Embed a 24-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 24-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_24(uint8_t *ptr, uint32_t val)
{
    ptr[0] = (val >> 16) & 0xff;
    ptr[1] = (val >> 8) & 0xff;
    ptr[2] = val & 0xff;
}

/**
 * Extract a 32-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 32-bit number from the beginning of a buffer.
 */
inline
uint32_t
extract_32(const uint8_t *ptr)
{
    uint32_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];
    val <<= 8;
    val |= ptr[2];
    val <<= 8;
    val |= ptr[3];

    return val;
}

/**
 * Embed a 32-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 32-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_32(uint8_t *ptr, uint32_t val)
{
    ptr[0] = (val >> 24) & 0xff;
    ptr[1] = (val >> 16) & 0xff;
    ptr[2] = (val >> 8) & 0xff;
    ptr[3] = val & 0xff;
}

/**
 * Extract an 8-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer with the data.
 * @return an 8-bit number from the beginning of a buffer.
 */
inline
uint8_t
extract_host_8(const uint8_t *ptr)
{
    uint8_t val;

    val = ptr[0];

    return val;
}

/**
 * Embed an 8-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 8-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_host_8(uint8_t *ptr, uint8_t val)
{
    ptr[0] = val;
}

/**
 * Extract a 16-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer with the data.
 * @return a 16-bit number from the beginning of a buffer.
 */
inline
uint16_t
extract_host_16(const uint8_t *ptr)
{
    union {
	uint16_t val;
	uint8_t  c[sizeof(uint16_t)];
    } u;

    u.c[0] = ptr[0];
    u.c[1] = ptr[1];

    return u.val;
}

/**
 * Embed a 16-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 16-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_host_16(uint8_t *ptr, uint16_t val)
{
    union {
	uint16_t val;
	uint8_t  c[sizeof(uint16_t)];
    } u;

    u.val = val;
    ptr[0] = u.c[0];
    ptr[1] = u.c[1];
}

/*
 * XXX: Note that we don't define extract_host_24() and embed_host_24(),
 * because 3 octets of data might occupy either 3 or 4 octets (depending
 * on the byte ordering). Hence, extract_host_32() and embed_host_32()
 * should be used instead.
 */

/**
 * Extract a 32-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer with the data.
 * @return a 32-bit number from the beginning of a buffer.
 */
inline
uint32_t
extract_host_32(const uint8_t *ptr)
{
    union {
	uint32_t val;
	uint8_t  c[sizeof(uint32_t)];
    } u;

    u.c[0] = ptr[0];
    u.c[1] = ptr[1];
    u.c[2] = ptr[2];
    u.c[3] = ptr[3];

    return u.val;
}

/**
 * Embed a 32-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 32-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_host_32(uint8_t *ptr, uint32_t val)
{
    union {
	uint32_t val;
	uint8_t  c[sizeof(uint32_t)];
    } u;

    u.val = val;
    ptr[0] = u.c[0];
    ptr[1] = u.c[1];
    ptr[2] = u.c[2];
    ptr[3] = u.c[3];
}

/**
 * Extract an integer number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer with the data.
 * @return an integer number from the beginning of a buffer.
 */
inline
int
extract_host_int(const uint8_t *ptr)
{
    union {
	int val;
	uint8_t  c[sizeof(int)];
    } u;

    for (size_t i = 0; i < sizeof(int); i++) {
	u.c[i] = ptr[i];
    }

    return u.val;
}

/**
 * Embed an integer number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in host order.
 *
 * @param ptr the buffer to store the data.
 * @param val the integer value to embed into the beginning of the buffer.
 */
inline
void
embed_host_int(uint8_t *ptr, int val)
{
    union {
	int val;
	uint8_t  c[sizeof(int)];
    } u;

    u.val = val;
    for (size_t i = 0; i < sizeof(int); i++) {
	ptr[i] = u.c[i];
    }
}

/**
 * @short IPv4 packet header.
 *
 * The IPv4 packet header has the following content:
 *
 * ip_vhl (1 byte):	// IP ver/hdrlen (version << 4 | header length >> 2)
 * ip_tos (1 byte):	// Type of service
 * ip_len (2 bytes):	// Total length
 * ip_id (2 bytes):	// Identification
 * ip_off (2 bytes):	// Fragment offset field (least-significant 13 bits)
 * ip_ttl (1 byte):	// Time to live
 * ip_p (1 byte):	// Protocol
 * ip_sum (2 bytes):	// Checksum
 * ip_src (4 bytes):	// Source address
 * ip_dst (4 bytes):	// Destination address
 */
class IpHeader4 {
public:
    IpHeader4(const uint8_t* data)
	: _data(data),
	  _ip_vhl(_data + _ip_vhl_offset),
	  _ip_tos(_data + _ip_tos_offset),
	  _ip_len(_data + _ip_len_offset),
	  _ip_id(_data + _ip_id_offset),
	  _ip_off(_data + _ip_off_offset),
	  _ip_ttl(_data + _ip_ttl_offset),
	  _ip_p(_data + _ip_p_offset),
	  _ip_sum(_data + _ip_sum_offset),
	  _ip_src(_data + _ip_src_offset),
	  _ip_dst(_data + _ip_dst_offset)
    {
	static_assert(IpHeader4::SIZE == _ip_vhl_sizeof + _ip_tos_sizeof
		      + _ip_len_sizeof + _ip_id_sizeof + _ip_off_sizeof
		      + _ip_ttl_sizeof + _ip_p_sizeof + _ip_sum_sizeof
		      + _ip_src_sizeof + _ip_dst_sizeof);
	static_assert(IpHeader4::SIZE == _ip_dst_offset + _ip_dst_sizeof);
    }

    static const size_t SIZE = 20;		// The header size
    static const uint8_t IP_VERSION = 4;	// IPv4 version

    /**
     * Get the IPv4 packet header size.
     *
     * Note that this is the header size only without any header options.
     *
     * @return the IPv4 packet header size.
     */
    static size_t size() { return IpHeader4::SIZE; }

    /**
     * Get the buffer data.
     *
     * @return the buffer data.
     */
    const uint8_t* data() const { return (_data); }

    /**
     * Methods to get various IP header fields.
     */
    uint8_t  ip_vhl() const	{ return extract_8(_ip_vhl); }
    uint8_t  ip_tos() const	{ return extract_8(_ip_tos); }
    uint16_t ip_len() const	{ return extract_16(_ip_len); }
    uint16_t ip_id() const	{ return extract_16(_ip_id); }
    uint16_t ip_off() const	{ return extract_16(_ip_off); }
    uint8_t  ip_ttl() const	{ return extract_8(_ip_ttl); }
    uint8_t  ip_p() const	{ return extract_8(_ip_p); }
    uint16_t ip_sum() const	{ return extract_16(_ip_sum); }
    IPv4     ip_src() const	{ return IPv4(_ip_src); }
    IPv4     ip_dst() const	{ return IPv4(_ip_dst); }

    /*
     * A method to extract the ip_len value that is presumably stored
     * in host order.
     *
     * @return the ip_len value that is presumably stored in host order.
     */
    uint16_t ip_len_host() const{ return extract_host_16(_ip_len); }

    /**
     * Get the IP protocol version of the header.
     *
     * @return the IP protocol version of the header.
     */
    uint8_t ip_version() const {
	uint8_t v = ip_vhl();
	return ((v >> 4) & 0x0f);
    }

    /**
     * Get the IPv4 packet header size (including any header options).
     *
     * @return the IPv4 packet header size (including any header options).
     */
    uint8_t ip_header_len() const { return ((ip_vhl() & 0x0f) << 2); }

    /**
     * Get the IPv4 fragment offset (excluding the fragment flags).
     *
     * @return the IPv4 fragment offset (excluding the fragment flags).
     */
    uint16_t ip_fragment_offset() const {
	return (ip_off() & IpHeader4::FRAGMENT_OFFSET_MASK);
    }

    /**
     * Get the IPv4 fragment flags.
     *
     * @return the IPv4 fragment flags.
     */
    uint16_t ip_fragment_flags() const {
	return (ip_off() & IpHeader4::FRAGMENT_FLAGS_MASK);
    }

    /**
     * Test whether the IP header version is valid.
     *
     * @return true if the IP header version is valid, otherwise false.
     */
    bool is_valid_version() const {
	return (ip_version() == IpHeader4::IP_VERSION);
    }

    /**
     * Fragment an IPv4 packet.
     *
     * Note: If the original packet is not larger than the MTU, then the packet
     * is not fragmented (i.e., @ref fragments is empty), and the return
     * value is XORP_OK.
     *
     * @param mtu the MTU for fragmenting the packet.
     * @param fragments the return-by-reference fragments of the IPv4 packet.
     * @param do_checksum if true, compute and set the checksum in the IPv4
     * header, otherwise reset it to zero.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int fragment(size_t mtu, list<vector<uint8_t> >& fragments,
		 bool do_checksum, string& error_msg) const;

protected:
    // IPv4 header related constants
    static const uint16_t FRAGMENT_OFFSET_MASK	= 0x1fff;
    static const uint16_t FRAGMENT_FLAGS_MASK	= 0xe000;
    static const uint16_t FRAGMENT_FLAGS_IP_DF	= 0x4000; // Don't fragment
    static const uint16_t FRAGMENT_FLAGS_IP_MF	= 0x2000; // More fragments
    static const uint8_t  OPTIONS_IPOPT_EOL	= 0;	// End of option list
    static const uint8_t  OPTIONS_IPOPT_NOP	= 1;	// No operation
    static const size_t	  OPTIONS_IPOPT_OLEN	= 1;	// Option length offset
    static const uint8_t  OPTIONS_COPIED_FLAG	= 0x80;	// Option copied flag

    // Sizes of the fields
    static const size_t _ip_vhl_sizeof	= 1;
    static const size_t _ip_tos_sizeof	= 1;
    static const size_t _ip_len_sizeof	= 2;
    static const size_t _ip_id_sizeof	= 2;
    static const size_t _ip_off_sizeof	= 2;
    static const size_t _ip_ttl_sizeof	= 1;
    static const size_t _ip_p_sizeof	= 1;
    static const size_t _ip_sum_sizeof	= 2;
    static const size_t _ip_src_sizeof	= 4;
    static const size_t _ip_dst_sizeof	= 4;

    // Offsets for the fields
    static const size_t _ip_vhl_offset	= 0;
    static const size_t _ip_tos_offset	= _ip_vhl_offset + _ip_vhl_sizeof;
    static const size_t _ip_len_offset	= _ip_tos_offset + _ip_tos_sizeof;
    static const size_t _ip_id_offset	= _ip_len_offset + _ip_len_sizeof;
    static const size_t _ip_off_offset	= _ip_id_offset + _ip_id_sizeof;
    static const size_t _ip_ttl_offset	= _ip_off_offset + _ip_off_sizeof;
    static const size_t _ip_p_offset	= _ip_ttl_offset + _ip_ttl_sizeof;
    static const size_t _ip_sum_offset	= _ip_p_offset + _ip_p_sizeof;
    static const size_t _ip_src_offset	= _ip_sum_offset + _ip_sum_sizeof;
    static const size_t _ip_dst_offset	= _ip_src_offset + _ip_src_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _ip_vhl;	// IP version and header length
    const uint8_t* _ip_tos;	// Type of service
    const uint8_t* _ip_len;	// Total length
    const uint8_t* _ip_id;	// Identification
    const uint8_t* _ip_off;	// Fragment offset field
    const uint8_t* _ip_ttl;	// Time to live
    const uint8_t* _ip_p;	// Protocol
    const uint8_t* _ip_sum;	// Checksum
    const uint8_t* _ip_src;	// Source address
    const uint8_t* _ip_dst;	// Destination address
};

/**
 * @short Class for writing data to IPv4 packet header.
 */
class IpHeader4Writer : public IpHeader4 {
public:
    IpHeader4Writer(uint8_t* data)
	: IpHeader4(data),
	  _data(data),
	  _ip_vhl(_data + _ip_vhl_offset),
	  _ip_tos(_data + _ip_tos_offset),
	  _ip_len(_data + _ip_len_offset),
	  _ip_id(_data + _ip_id_offset),
	  _ip_off(_data + _ip_off_offset),
	  _ip_ttl(_data + _ip_ttl_offset),
	  _ip_p(_data + _ip_p_offset),
	  _ip_sum(_data + _ip_sum_offset),
	  _ip_src(_data + _ip_src_offset),
	  _ip_dst(_data + _ip_dst_offset)
    {}

    /**
     * Get the buffer data.
     *
     * @return the buffer data.
     */
    uint8_t* data() { return (_data); }

    /**
     * Methods to set various IP header fields.
     */
    void set_ip_vhl(uint8_t v)		{ embed_8(_ip_vhl, v); }
    void set_ip_tos(uint8_t v)		{ embed_8(_ip_tos, v); }
    void set_ip_len(uint16_t v)		{ embed_16(_ip_len, v); }
    void set_ip_id(uint16_t v)		{ embed_16(_ip_id, v); }
    void set_ip_off(uint16_t v)		{ embed_16(_ip_off, v); }
    void set_ip_ttl(uint8_t v)		{ embed_8(_ip_ttl, v); }
    void set_ip_p(uint8_t v)		{ embed_8(_ip_p, v); }
    void set_ip_sum(uint16_t v)		{ embed_16(_ip_sum, v); }
    void set_ip_src(const IPv4& v)	{ v.copy_out(_ip_src); }
    void set_ip_dst(const IPv4& v)	{ v.copy_out(_ip_dst); }

    /*
     * A method to embed the ip_len value by storing it in host order.
     *
     * @param v the ip_len value that will be stored in host order.
     */
    void set_ip_len_host(uint16_t v)	{ embed_host_16(_ip_len, v); }

    /**
     * Set the IP protocol version of the header.
     *
     * @param v the IP protocol version of the header.
     */
    void set_ip_version(uint8_t v) {
	uint8_t vhl = ((v << 4) | (ip_header_len() >> 2));
	set_ip_vhl(vhl);
    }

    /**
     * Set the IPv4 packet header size (including any header options).
     *
     * @param v the IPv4 packet header size (including any header options).
     */
    void set_ip_header_len(uint8_t v) {
	uint8_t vhl = ((ip_version() << 4) | (v >> 2));
	set_ip_vhl(vhl);
    }

    /**
     * Set the IPv4 fragment offset (excluding the fragment flags).
     *
     * @param v the IPv4 fragment offset (excluding the fragment flags).
     */
    void set_ip_fragment_offset(uint16_t v) {
	uint16_t off = v & IpHeader4::FRAGMENT_OFFSET_MASK;
	off |= ip_fragment_flags();
	set_ip_off(off);
    }

    /**
     * Set the IPv4 fragment flags.
     *
     * @param v the IPv4 fragment flags.
     */
    void set_ip_fragment_flags(uint16_t v) {
	uint16_t off = v & IpHeader4::FRAGMENT_FLAGS_MASK;
	off |= ip_fragment_offset();
	set_ip_off(off);
    }

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _ip_vhl;		// IP version and header length
    uint8_t* _ip_tos;		// Type of service
    uint8_t* _ip_len;		// Total length
    uint8_t* _ip_id;		// Identification
    uint8_t* _ip_off;		// Fragment offset field
    uint8_t* _ip_ttl;		// Time to live
    uint8_t* _ip_p;		// Protocol
    uint8_t* _ip_sum;		// Checksum
    uint8_t* _ip_src;		// Source address
    uint8_t* _ip_dst;		// Destination address
};

/**
 * @short IPv6 packet header.
 *
 * The IPv6 packet header has the following content:
 *
 * ip_vtc_flow (4 bytes): // 4 bits vers., 8 bits traf. class, 20 bits flow-ID
 * ip_plen (2 bytes):	// Payload length
 * ip_nxt (1 byte):	// Next header
 * ip_hlim (1 byte):	// Hop limit
 * ip_src (16 bytes):	// Source address
 * ip_dst (16 bytes):	// Destination address
 */
class IpHeader6 {
public:
    IpHeader6(const uint8_t* data)
	: _data(data),
	  _ip_vtc_flow(_data + _ip_vtc_flow_offset),
	  _ip_plen(_data + _ip_plen_offset),
	  _ip_nxt(_data + _ip_nxt_offset),
	  _ip_hlim(_data + _ip_hlim_offset),
	  _ip_src(_data + _ip_src_offset),
	  _ip_dst(_data + _ip_dst_offset)
    {
	static_assert(IpHeader6::SIZE == _ip_vtc_flow_sizeof
		      + _ip_plen_sizeof + _ip_nxt_sizeof + _ip_hlim_sizeof
		      + _ip_src_sizeof + _ip_dst_sizeof);
	static_assert(IpHeader6::SIZE == _ip_dst_offset + _ip_dst_sizeof);
    }

    static const size_t SIZE = 40;		// The header size
    static const uint8_t IP_VERSION = 6;	// IPv6 version

    /**
     * Get the IPv6 packet header size.
     *
     * Note that this is the header size only without any extention headers.
     *
     * @return the IPv6 packet header size.
     */
    static size_t size() { return IpHeader6::SIZE; }

    /**
     * Get the buffer data.
     *
     * @return the buffer data.
     */
    const uint8_t* data() const { return (_data); }

    /**
     * Methods to get various IP header fields.
     */
    uint32_t ip_vtc_flow() const { return extract_32(_ip_vtc_flow); }
    uint16_t ip_plen() const	{ return extract_16(_ip_plen); }
    uint8_t  ip_nxt() const	{ return extract_8(_ip_nxt); }
    uint8_t  ip_hlim() const	{ return extract_8(_ip_hlim); }
    IPv6     ip_src() const	{ return IPv6(_ip_src); }
    IPv6     ip_dst() const	{ return IPv6(_ip_dst); }

    /**
     * Get the IP protocol version of the header.
     *
     * @return the IP protocol version of the header.
     */
    uint8_t ip_version() const {
	uint32_t v = ip_vtc_flow() & IpHeader6::VERSION_MASK;
	return ((v >> IpHeader6::VERSION_SHIFT) & 0x0f);
    }

    /**
     * Get the IPv6 traffic class.
     *
     * @return the IPv6 traffic class.
     */
    uint8_t ip_traffic_class() const {
	uint32_t tc = ip_vtc_flow() & IpHeader6::TRAFFIC_CLASS_MASK;
	return ((tc >> IpHeader6::TRAFFIC_CLASS_SHIFT) & 0xff);
    }

    /**
     * Get the IPv6 flow label.
     *
     * @return the IPv6 flow label.
     */
    uint32_t ip_flow_label() const {
	uint32_t flow = ip_vtc_flow() & IpHeader6::FLOW_LABEL_MASK;
	return (flow >> IpHeader6::FLOW_LABEL_SHIFT);
    }

    /**
     * Test whether the IP header version is valid.
     *
     * @return true if the IP header version is valid, otherwise false.
     */
    bool is_valid_version() const {
	return (ip_version() == IpHeader6::IP_VERSION);
    }

protected:
    static const uint32_t VERSION_MASK		= 0xf0000000;
    static const uint32_t TRAFFIC_CLASS_MASK	= 0x0ff00000;
    static const uint32_t FLOW_LABEL_MASK	= 0x000fffff;
    static const size_t   VERSION_SHIFT		= 28;
    static const size_t   TRAFFIC_CLASS_SHIFT	= 20;
    static const size_t   FLOW_LABEL_SHIFT	= 0;

    // Sizes of the fields
    static const size_t _ip_vtc_flow_sizeof	= 4;
    static const size_t _ip_plen_sizeof		= 2;
    static const size_t _ip_nxt_sizeof		= 1;
    static const size_t _ip_hlim_sizeof		= 1;
    static const size_t _ip_src_sizeof		= 16;
    static const size_t _ip_dst_sizeof		= 16;

    // Offsets for the fields
    static const size_t _ip_vtc_flow_offset	= 0;
    static const size_t _ip_plen_offset	= _ip_vtc_flow_offset + _ip_vtc_flow_sizeof;
    static const size_t _ip_nxt_offset	= _ip_plen_offset + _ip_plen_sizeof;
    static const size_t _ip_hlim_offset	= _ip_nxt_offset + _ip_nxt_sizeof;
    static const size_t _ip_src_offset	= _ip_hlim_offset + _ip_hlim_sizeof;
    static const size_t _ip_dst_offset	= _ip_src_offset + _ip_src_sizeof;

private:
    const uint8_t* _data;	// The buffer data

    // Pointers to the fields
    const uint8_t* _ip_vtc_flow; // IP version, traffic class and flow label
    const uint8_t* _ip_plen;	// Payload length
    const uint8_t* _ip_nxt;	// Next header
    const uint8_t* _ip_hlim;	// Hop limit
    const uint8_t* _ip_src;	// Source address
    const uint8_t* _ip_dst;	// Destination address
};

/**
 * @short Class for writing data to IPv6 packet header.
 */
class IpHeader6Writer : public IpHeader6 {
public:
    IpHeader6Writer(uint8_t* data)
	: IpHeader6(data),
	  _data(data),
	  _ip_vtc_flow(_data + _ip_vtc_flow_offset),
	  _ip_plen(_data + _ip_plen_offset),
	  _ip_nxt(_data + _ip_nxt_offset),
	  _ip_hlim(_data + _ip_hlim_offset),
	  _ip_src(_data + _ip_src_offset),
	  _ip_dst(_data + _ip_dst_offset)
    {}

    /**
     * Get the buffer data.
     *
     * @return the buffer data.
     */
    uint8_t* data() { return (_data); }

    /**
     * Methods to set various IP header fields.
     */
    void set_ip_vtc_flow(uint32_t v)	{ embed_32(_ip_vtc_flow, v); }
    void set_ip_plen(uint16_t v)	{ embed_16(_ip_plen, v); }
    void set_ip_nxt(uint8_t v)		{ embed_8(_ip_nxt, v); }
    void set_ip_hlim(uint8_t v)		{ embed_8(_ip_hlim, v); }
    void set_ip_src(const IPv6& v)	{ v.copy_out(_ip_src); }
    void set_ip_dst(const IPv6& v)	{ v.copy_out(_ip_dst); }

    /**
     * Set the IP protocol version of the header.
     *
     * @param v the IP protocol version of the header.
     */
    void set_ip_version(uint8_t v) {
	uint32_t vtc_flow = ip_vtc_flow() & ~IpHeader6::VERSION_MASK;
	uint32_t shifted_v = v;
	shifted_v <<= IpHeader6::VERSION_SHIFT;
	vtc_flow |= shifted_v;
	set_ip_vtc_flow(vtc_flow);
    }

    /**
     * Set the IPv6 traffic class.
     *
     * @param v the IPv6 traffic class.
     */
    void set_ip_traffic_class(uint8_t v) {
	uint32_t vtc_flow = ip_vtc_flow() & ~IpHeader6::TRAFFIC_CLASS_MASK;
	uint32_t shifted_v = v;
	shifted_v <<= IpHeader6::TRAFFIC_CLASS_SHIFT;
	vtc_flow |= shifted_v;
	set_ip_vtc_flow(vtc_flow);
    }

    /**
     * Set the IPv6 flow label.
     *
     * @param v the IPv6 flow label.
     */
    void set_ip_flow_label(uint32_t v) {
	uint32_t vtc_flow = ip_vtc_flow() & ~IpHeader6::FLOW_LABEL_MASK;
	uint32_t shifted_v = v;
	shifted_v <<= IpHeader6::FLOW_LABEL_SHIFT;
	vtc_flow |= shifted_v;
	set_ip_vtc_flow(vtc_flow);
    }

private:
    uint8_t* _data;		// The buffer data

    // Pointers to the fields
    uint8_t* _ip_vtc_flow;	// IP version, traffic class and flow label
    uint8_t* _ip_plen;		// Payload length
    uint8_t* _ip_nxt;		// Next header
    uint8_t* _ip_hlim;		// Hop limit
    uint8_t* _ip_src;		// Source address
    uint8_t* _ip_dst;		// Destination address
};

class BadPacketException : public XorpReasonedException {
public:
    BadPacketException(const char* file, size_t line, const string& why = "")
        : XorpReasonedException("BadPacketException", file, line, why) {}
};

struct ArpHeader {
    typedef vector<uint8_t> PAYLOAD;

    enum Op {
	ARP_REQUEST = 1,
	ARP_REPLY
    };
    enum HwFmt {
	HW_ETHER = 1
    };

    static ArpHeader&	    assign(uint8_t* data);
    static const ArpHeader& assign(const PAYLOAD& payload);
    static void		    make_gratuitous(PAYLOAD& payload, const Mac& mac,
					    const IPv4& ip);
    void		    set_sender(const Mac& mac, const IPv4& ip);
    void		    set_request(const IPv4& ip);
    void		    set_reply(const Mac& mac, const IPv4& ip);
    uint32_t		    size() const;
    bool		    is_request() const;
    IPv4		    get_request() const;
    void		    make_reply(PAYLOAD& out, const Mac& mac) const;

    uint16_t	ah_hw_fmt;
    uint16_t	ah_proto_fmt;
    uint8_t	ah_hw_len;
    uint8_t	ah_proto_len;
    uint16_t	ah_op;
    uint8_t	ah_data[0];
};

#endif // __LIBPROTO_PACKET_HH__
