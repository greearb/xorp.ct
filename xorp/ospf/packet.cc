// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
// #define DEBUG_RAW_PACKETS

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libproto/checksum.h"
#include "libproto/packet.hh"






#include "ospf.hh"
#include "packet.hh"

/**
 * Return the IP checksum in host order.
 */
inline
uint16_t
checksum(uint8_t *ptr, size_t len)
{
    return ntohs(inet_checksum(ptr, len));
}

/**
 * Return the IPv6 pseudo header checksum in host order.
 */
inline
uint16_t
ipv6_pseudo_header_checksum(const IPv6& src, const IPv6& dst, size_t len,
			    uint8_t protocol)
{
    uint8_t pseudo_header[16	/* Source address */
			  + 16	/* Destination address */
			  + 4 	/* Upper-layer packet length */
			  + 3 	/* Zero */
			  + 1	/* Upper-layer protocol number */
			  ];

    src.copy_out(&pseudo_header[0]);
    dst.copy_out(&pseudo_header[16]);
    embed_32(&pseudo_header[16 + 16], len);
    embed_24(&pseudo_header[16 + 16 + 4], 0);
    pseudo_header[16 + 16 + 4 + 3] = protocol;

    return checksum(&pseudo_header[0], sizeof(pseudo_header));
}

template <>
void
ipv6_checksum_verify<IPv6>(const IPv6& src, const IPv6& dst,
			   const uint8_t *data, size_t len,
			   size_t checksum_offset,
			   uint8_t protocol) throw(InvalidPacket)
{
    debug_msg("src %s dst data %p %s len %u chsum offset %u protocol %u\n",
	      cstring(src), cstring(dst),
	      data, XORP_UINT_CAST(len), XORP_UINT_CAST(checksum_offset),
	      protocol);

    if (len < checksum_offset)
	xorp_throw(InvalidPacket,
		   c_format("Checksum offset %u greater than packet length %u",
			    XORP_UINT_CAST(checksum_offset),
			    XORP_UINT_CAST(len)));

    if (0 == inet_checksum_add(ipv6_pseudo_header_checksum(src, dst, len,
							   protocol),
			       checksum(const_cast<uint8_t *>(data), len)))
	return;

    // If we get here there is a problem with the checksum. Compute
    // the expected checksum to aid in debugging.
    uint8_t *temp = new uint8_t[len];
    memcpy(&temp[0], &data[0], len);
    uint16_t checksum_inpacket = extract_16(&temp[checksum_offset]);
    embed_16(&temp[checksum_offset], 0);
    uint16_t checksum_computed =
	inet_checksum_add(ipv6_pseudo_header_checksum(src, dst, len,
							   protocol),
			  checksum(temp, len));
    delete []temp;
						 ;
    if (checksum_inpacket != checksum_computed)
	xorp_throw(InvalidPacket,
		   c_format("Checksum mismatch expected %#x received %#x",
			    checksum_computed,
			    checksum_inpacket));
}

template <>
void
ipv6_checksum_verify<IPv4>(const IPv4& src, const IPv4& dst,
			   const uint8_t *data, size_t len,
			   size_t checksum_offset,
			   uint8_t protocol) throw(InvalidPacket)
{
    debug_msg("src %s dst data %p %s len %u chsum offset %u protocol %u\n",
	      cstring(src), cstring(dst),
	      data, XORP_UINT_CAST(len), XORP_UINT_CAST(checksum_offset),
	      protocol);
}

template <>
void
ipv6_checksum_apply<IPv6>(const IPv6& src, const IPv6& dst,
			  uint8_t *data, size_t len,
			  size_t checksum_offset,
			  uint8_t protocol) throw(InvalidPacket)
{
    debug_msg("src %s dst data %p %s len %u chsum offset %u protocol %u\n",
	      cstring(src), cstring(dst),
	      data, XORP_UINT_CAST(len), XORP_UINT_CAST(checksum_offset),
	      protocol);

    if (len < checksum_offset)
	xorp_throw(InvalidPacket,
		   c_format("Checksum offset %u greater than packet length %u",
			    XORP_UINT_CAST(checksum_offset),
			    XORP_UINT_CAST(len)));

    embed_16(&data[checksum_offset],
	     inet_checksum_add(ipv6_pseudo_header_checksum(src, dst, len,
							   protocol),
			       checksum(data, len)));
}

template <>
void
ipv6_checksum_apply<IPv4>(const IPv4& src, const IPv4& dst,
			  uint8_t *data, size_t len,
			  size_t checksum_offset,
			  uint8_t protocol) throw(InvalidPacket)
{
    debug_msg("src %s dst data %p %s len %u chsum offset %u protocol %u\n",
	      cstring(src), cstring(dst),
	      data, XORP_UINT_CAST(len), XORP_UINT_CAST(checksum_offset),
	      protocol);
}

#ifdef	DEBUG_RAW_PACKETS
inline
string
dump_packet(uint8_t *ptr, size_t len)
{
    string output;

    for(size_t i = 0; i < len; i++) {
	output += c_format("%#4x ", ptr[i]);
	if (!((i + 1) % 4))
	    output += "\n";
    }

    return output;
}
#endif

/* Packet */

size_t
Packet::decode_standard_header(uint8_t *ptr, size_t& len) throw(InvalidPacket)
{
    debug_msg("ptr %p len %u\n", ptr, XORP_UINT_CAST(len));
#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    // Store a copy of the raw packet data for possible later use for
    // authentication.
    store(ptr, len);

    // Make sure that at least two bytes have been extracted:
    // Version and Type fields.
    if (len < 2)
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(2)));

    OspfTypes::Version version;
    switch(ptr[0]) {
    case 2:
	version = OspfTypes::V2;
	break;
    case 3:
	version = OspfTypes::V3;
	break;
    default:
	xorp_throw(InvalidPacket,
		   c_format("Version mismatch expected %u received %u",
			    get_version(),
			    ptr[Packet::VERSION_OFFSET] & 0xff));
	break;
    }

    if (ptr[1] != get_type())
	xorp_throw(InvalidPacket,
		   c_format("Type mismatch expected %u received %u",
			    get_type(),
			    ptr[Packet::TYPE_OFFSET]));

    // Make sure that at least the standard header length is present.
    switch(version) {
    case OspfTypes::V2:
	if (len < STANDARD_HEADER_V2)
	    xorp_throw(InvalidPacket,
		       c_format("Packet too short %u, must be at least %u",
				XORP_UINT_CAST(len),
				XORP_UINT_CAST(STANDARD_HEADER_V2)));
	break;
    case OspfTypes::V3:
	if (len < STANDARD_HEADER_V3)
	    xorp_throw(InvalidPacket,
		       c_format("Packet too short %u, must be at least %u",
				XORP_UINT_CAST(len),
				XORP_UINT_CAST(STANDARD_HEADER_V3)));
	break;
    }

    // Verify that the length in the packet and the length of received
    // data match.
    uint32_t packet_length = extract_16(&ptr[Packet::LEN_OFFSET]);
    if (packet_length != len) {
	// If the frame is too small complain.
	if (len < packet_length)
	    xorp_throw(InvalidPacket,
		       c_format("Packet length expected %u received %u",
			    packet_length,
			    XORP_UINT_CAST(len)));
	// "Be liberal in what you accept, and conservative in what you send."
	// -- Jon Postel
	len = packet_length;	// Drop the length and continue.
    }

    set_router_id(extract_32(&ptr[Packet::ROUTER_ID_OFFSET]));
    set_area_id(extract_32(&ptr[Packet::AREA_ID_OFFSET]));

    // In OSPFv2 there is authentication info in the standard header.
    switch(version) {
    case OspfTypes::V2:
	// Verify the auth structure is the correct size.
	x_static_assert(sizeof(_auth) == (64 / 8));
	set_auth_type(extract_16(&ptr[Packet::AUTH_TYPE_OFFSET]));
	memcpy(&_auth[0], &ptr[Packet::AUTH_PAYLOAD_OFFSET], sizeof(_auth));
	// The authentication field is expected to be zero for the
	// checksumming.
	memset(&ptr[Packet::AUTH_PAYLOAD_OFFSET], 0, sizeof(_auth));
	break;
    case OspfTypes::V3:
	set_instance_id(ptr[Packet::INSTANCE_ID_OFFSET]);
	// For OSPFv3 the checksum has already been verified.
	return get_standard_header_length();
	break;
    }

    // Extract the checksum and check the packet.
    uint16_t checksum_inpacket = extract_16(&ptr[Packet::CHECKSUM_OFFSET]);

    uint16_t verify_checksum = checksum(ptr, len);

    // Restore the zero'd fields.
    switch(version) {
    case OspfTypes::V2:
	memcpy(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_auth[0], sizeof(_auth));
	break;
    case OspfTypes::V3:
	break;
    }

    if (0 == checksum_inpacket &&
	OspfTypes::CRYPTOGRAPHIC_AUTHENTICATION == get_auth_type())
	return get_standard_header_length();

    if (verify_checksum != 0) {
	//Calculate valid checksum for debugging purposes

	// Zero the checksum location.
	embed_16(&ptr[Packet::CHECKSUM_OFFSET], 0);
	uint16_t checksum_actual = checksum(ptr, len);
	xorp_throw(InvalidPacket,
		   c_format("Checksum mismatch expected %#x received %#x",
			    checksum_actual,
			    checksum_inpacket));
    }

    // Return the offset at which continued processing can take place.
    return get_standard_header_length();
}

size_t
Packet::encode_standard_header(uint8_t *ptr, size_t len)
{
    debug_msg("ptr %p len %u\n", ptr, XORP_UINT_CAST(len));

    if (len < get_standard_header_length()) {
	XLOG_ERROR("Request to put a header of size %u in space %u",
		   XORP_UINT_CAST(get_standard_header_length()),
		   XORP_UINT_CAST(len));
	return 0;
    }

    // Zero the space
    memset(ptr, 0, get_standard_header_length());

    OspfTypes::Version version = get_version();

    ptr[Packet::VERSION_OFFSET] = version;
    ptr[Packet::TYPE_OFFSET] = get_type();
    embed_16(&ptr[Packet::LEN_OFFSET], len);
    embed_32(&ptr[Packet::ROUTER_ID_OFFSET], get_router_id());
    embed_32(&ptr[Packet::AREA_ID_OFFSET], get_area_id());

    switch(version) {
    case OspfTypes::V2:
	embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], get_auth_type());
	break;
    case OspfTypes::V3:
	ptr[Packet::INSTANCE_ID_OFFSET] = get_instance_id();
	// For OSPFv3 the checksum will be written later.
	return get_standard_header_length();
	break;
    }

    embed_16(&ptr[Packet::CHECKSUM_OFFSET], checksum(ptr, len));

    // OSPFv2 only copy the authentication data out.
    switch(version) {
    case OspfTypes::V2:
	memcpy(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_auth[0], sizeof(_auth));
	break;
    case OspfTypes::V3:
	break;
    }

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    return get_standard_header_length();
}

string
Packet::standard() const
{
    string output;

    output = c_format("\tVersion %u\n", get_version());
    output += c_format("\tType %u\n", get_type());
    output += "\tRouter ID " + pr_id(get_router_id()) + "\n";
    output += "\tArea ID " + pr_id(get_area_id()) + "\n";

    switch(get_version()) {
    case OspfTypes::V2:
	output += c_format("\tAuth Type %u", get_auth_type());
	break;
    case OspfTypes::V3:
	output += c_format("\tInstance ID %u", get_instance_id());
	break;
    }

    return output;
}

/* PacketDecoder */

PacketDecoder::~PacketDecoder()
{
    // Free all the stored decoder packets.
    map<OspfTypes::Type , Packet *>::iterator i;

    for(i = _ospfv2.begin(); i != _ospfv2.end(); i++)
	delete i->second;

    for(i = _ospfv3.begin(); i != _ospfv3.end(); i++)
	delete i->second;
}

#if	0
void
PacketDecoder::register_decoder(Packet *packet, OspfTypes::Version version,
				OspfTypes::Type type)
{
    switch(version) {
    case OspfTypes::V2:
	_ospfv2[type] = packet;
	break;
    case OspfTypes::V3:
	_ospfv3[type] = packet;
	break;
    }
}
#endif

void
PacketDecoder::register_decoder(Packet *packet)
{
    switch(packet->get_version()) {
    case OspfTypes::V2:
	// Don't allow a registration to be overwritten.
	XLOG_ASSERT(0 == _ospfv2.count(packet->get_type()));
	_ospfv2[packet->get_type()] = packet;
	break;
    case OspfTypes::V3:
	// Don't allow a registration to be overwritten.
	XLOG_ASSERT(0 == _ospfv3.count(packet->get_type()));
	_ospfv3[packet->get_type()] = packet;
	break;
    }
}

Packet *
PacketDecoder::decode(uint8_t *ptr, size_t len) throw(InvalidPacket)
{
    // Make sure that at least two bytes have been extracted:
    // Version and Type fields.
    if (len < 2)
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(Packet::TYPE_OFFSET)));

    OspfTypes::Version version;
    switch(ptr[Packet::VERSION_OFFSET]) {
    case 2:
	version = OspfTypes::V2;
	break;
    case 3:
	version = OspfTypes::V3;
	break;
    default:
	xorp_throw(InvalidPacket,
		   c_format("Unknown OSPF Version %u",
			    ptr[Packet::VERSION_OFFSET] & 0xff));
	break;
    }

    map<OspfTypes::Type , Packet *>::iterator i;
    uint8_t type = ptr[Packet::TYPE_OFFSET];
    Packet *packet = NULL;
    switch(version) {
    case OspfTypes::V2:
	i = _ospfv2.find(type);
	if (i != _ospfv2.end())
	    packet = i->second;
	break;
    case OspfTypes::V3:
	i = _ospfv3.find(type);
	if (i != _ospfv3.end())
	    packet = i->second;
	break;
    }

    if (packet == NULL)
	xorp_throw(InvalidPacket,
		   c_format("OSPF Version %u Unknown Type %u", version, type));

    return packet->decode(ptr, len);
}

/* HelloPacket */

Packet *
HelloPacket::decode(uint8_t *ptr, size_t len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    HelloPacket *packet = new HelloPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);

    // Verify that this packet is large enough, up to but not including
    // any neighbours.
    if ((len - offset) < MINIMUM_LENGTH)
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + MINIMUM_LENGTH)));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    switch(version) {
    case OspfTypes::V2:
	packet->set_network_mask(extract_32(&ptr[offset +
					 HelloPacket::NETWORK_MASK_OFFSET]));
	packet->set_hello_interval(extract_16(&ptr[offset +
				   HelloPacket::HELLO_INTERVAL_V2_OFFSET]));
	packet->set_options(ptr[offset + HelloPacket::OPTIONS_V2_OFFSET]);
	packet->set_router_priority(ptr[offset +
				HelloPacket::ROUTER_PRIORITY_V2_OFFSET]);
	packet->set_router_dead_interval(extract_32(&ptr[offset +
			 HelloPacket::ROUTER_DEAD_INTERVAL_V2_OFFSET]));
	break;
    case OspfTypes::V3:
	packet->set_interface_id(extract_32(&ptr[offset +
					 HelloPacket::INTERFACE_ID_OFFSET]));
	packet->set_router_priority(ptr[offset +
				HelloPacket::ROUTER_PRIORITY_V3_OFFSET]);
	packet->set_options(extract_32(&ptr[offset +
			    HelloPacket::OPTIONS_V3_OFFSET]) & 0xffffff);
	packet->set_hello_interval(extract_16(&ptr[offset +
				   HelloPacket::HELLO_INTERVAL_V3_OFFSET]));
	packet->set_router_dead_interval(extract_16(&ptr[offset +
			 HelloPacket::ROUTER_DEAD_INTERVAL_V3_OFFSET]));
	break;
    }

     packet->set_designated_router(extract_32(&ptr[offset +
				   HelloPacket::DESIGNATED_ROUTER_OFFSET]));
     packet->set_backup_designated_router(extract_32(&ptr[offset +
			    HelloPacket::BACKUP_DESIGNATED_ROUTER_OFFSET]));

    // If there is any more space in the packet extract the neighbours.
    int neighbours = (len - (offset + MINIMUM_LENGTH)) / 4;

    // XXX - Should we be checking for multiples of 4 here?

    for(int i = 0; i < neighbours; i++)
	packet->get_neighbours().
	    push_back(extract_32(&ptr[offset + MINIMUM_LENGTH + i*4]));

    return packet;
}

bool
HelloPacket::encode(vector<uint8_t>& pkt)
{
    size_t offset = get_standard_header_length();
    size_t len = offset + MINIMUM_LENGTH + get_neighbours().size() * 4;

    pkt.resize(len);
    uint8_t *ptr = &pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Put the specific Hello Packet information first as the standard
    // header code will also add the checksum. This must be done last.

    /**************************************/
    OspfTypes::Version version = get_version();

    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[offset + HelloPacket::NETWORK_MASK_OFFSET],
		 get_network_mask());
	embed_16(&ptr[offset + HelloPacket::HELLO_INTERVAL_V2_OFFSET],
		 get_hello_interval());
	ptr[offset + HelloPacket::OPTIONS_V2_OFFSET] = get_options();
	ptr[offset + HelloPacket::ROUTER_PRIORITY_V2_OFFSET] =
	    get_router_priority();
	embed_32(&ptr[offset + HelloPacket::ROUTER_DEAD_INTERVAL_V2_OFFSET],
		 get_router_dead_interval());
	break;
    case OspfTypes::V3:
	embed_32(&ptr[offset + HelloPacket::INTERFACE_ID_OFFSET],
		 get_interface_id());
	// Careful Options occupy 3 bytes, four bytes are written out
	// and the top byte is overwritten by the router priority.
	embed_32(&ptr[offset + HelloPacket::OPTIONS_V3_OFFSET], get_options());
	ptr[offset + HelloPacket::ROUTER_PRIORITY_V3_OFFSET] =
	    get_router_priority();

	embed_16(&ptr[offset + HelloPacket::HELLO_INTERVAL_V3_OFFSET],
		 get_hello_interval());
	embed_16(&ptr[offset + HelloPacket::ROUTER_DEAD_INTERVAL_V3_OFFSET],
		 get_router_dead_interval());
	break;
    }

    embed_32(&ptr[offset + HelloPacket::DESIGNATED_ROUTER_OFFSET],
	     get_designated_router());
    embed_32(&ptr[offset + HelloPacket::BACKUP_DESIGNATED_ROUTER_OFFSET],
	     get_backup_designated_router());

    list<OspfTypes::RouterID> &li = get_neighbours();
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(size_t index = 0; i != li.end(); i++, index += 4) {
	embed_32(&ptr[offset + 20 + index], *i);
    }

    if (offset != encode_standard_header(ptr, len)) {
	XLOG_ERROR("Encode of %s failed", str().c_str());
	return false;
    }

    return true;
}

string
HelloPacket::str() const
{
    string output;

    output = "Hello Packet:\n";
    // Standard Header
    output += standard() + "\n";
    // Hello Packet Specifics

    switch(get_version()) {
    case OspfTypes::V2:
	output += c_format("\tNetwork Mask %#x\n", get_network_mask());
	break;
    case OspfTypes::V3:
	output += c_format("\tInterface ID %u\n", get_interface_id());
	break;
    }

    output += c_format("\tHello Interval %u\n", get_hello_interval());
    output += c_format("\tOptions %#x %s\n", get_options(),
		       cstring(Options(get_version(), get_options())));
    output += c_format("\tRouter Priority %u\n", get_router_priority());
    output += c_format("\tRouter Dead Interval %u\n",
		       get_router_dead_interval());
    output += "\tDesignated Router " + pr_id(get_designated_router()) +  "\n";
    output += "\tBackup Designated Router " +
	pr_id(get_backup_designated_router());

    list<OspfTypes::RouterID> li = _neighbours;
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(; i != li.end(); i++) {
	output += "\n\tNeighbour: " + pr_id(*i);
    }

    return output;
}


/* Database Description packet */

Packet *
DataDescriptionPacket::decode(uint8_t *ptr, size_t len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    DataDescriptionPacket *packet = new DataDescriptionPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);

    // Verify that this packet is large enough, up to but not including
    // any neighbours.
    if ((len - offset) < minimum_length())
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + minimum_length())));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    size_t bias = 0;

    switch(version) {
    case OspfTypes::V2:
	packet->set_interface_mtu(extract_16(&ptr[offset]));
	packet->set_options(ptr[offset + 2]);
	bias = 0;
	break;
    case OspfTypes::V3:
	packet->set_options(extract_32(&ptr[offset]) & 0xffffff);
	packet->set_interface_mtu(extract_16(&ptr[offset + 4]));
	bias = 4;
	break;
    }

    uint8_t flag = ptr[offset + bias + 3];
    if (flag & 0x4)
	packet->set_i_bit(true);
    else
	packet->set_i_bit(false);
    if (flag & 0x2)
	packet->set_m_bit(true);
    else
	packet->set_m_bit(false);
    if (flag & 0x1)
	packet->set_ms_bit(true);
    else
	packet->set_ms_bit(false);
    packet->set_dd_seqno(extract_32(&ptr[offset + bias + 4]));
    size_t lsa_offset = offset + 8 + bias;

    Lsa_header lsa_header(version);

    // If there is any more space in the packet extract the lsas.
    int lsas = (len - lsa_offset) / lsa_header.length();

    // XXX - Should we be checking for multiples of 20 here?
    for(int i = 0; i < lsas; i++) {
	packet->get_lsa_headers().
	    push_back(lsa_header.decode(&ptr[lsa_offset +
					     i*lsa_header.length()]));
    }

    return packet;
}

bool
DataDescriptionPacket::encode(vector<uint8_t>& pkt)
{
    size_t offset = get_standard_header_length();
    size_t len = offset + minimum_length() + get_lsa_headers().size() *
	Lsa_header::length();

    pkt.resize(len);
    uint8_t *ptr = &pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Put the specific Data Description Packet information first as
    // the standard header code will also add the checksum. This must
    // be done last.

    /**************************************/
    OspfTypes::Version version = get_version();

    size_t bias = 0;

    switch(version) {
    case OspfTypes::V2:
	embed_16(&ptr[offset], get_interface_mtu());
	ptr[offset + 2] = get_options();
	bias = 0;
	break;
    case OspfTypes::V3:
	// Careful Options occupy 3 bytes, four bytes are written out.
	embed_32(&ptr[offset], get_options());
	embed_16(&ptr[offset + 4], get_interface_mtu());
	bias = 4;
	break;
    }

    uint8_t flag = 0;
    if (get_i_bit())
	flag |= 0x4;
    if (get_m_bit())
	flag |= 0x2;
    if (get_ms_bit())
	flag |= 0x1;
    ptr[offset + bias + 3] = flag;

    embed_32(&ptr[offset + bias + 4], get_dd_seqno());
    size_t lsa_offset = offset + 8 + bias;

    list<Lsa_header> &li = get_lsa_headers();
    list<Lsa_header>::iterator i = li.begin();
    for(size_t index = 0; i != li.end(); i++, index += Lsa_header::length()) {
	(*i).copy_out(&ptr[lsa_offset + index]);
    }

    if (offset != encode_standard_header(ptr, len)) {
	XLOG_ERROR("Encode of %s failed", str().c_str());
	return false;
    }

    return true;
}

string
DataDescriptionPacket::str() const
{
    string output;

    output = "Data Description Packet:\n";
    // Standard Header
    output += standard() + "\n";
    // Data Description Packet Specifics

    output += c_format("\tInterface MTU %u\n", get_interface_mtu());
    output += c_format("\tOptions %#x %s\n", get_options(),
		       cstring(Options(get_version(), get_options())));
    output += c_format("\tI-bit %s\n", bool_c_str(get_i_bit()));
    output += c_format("\tM-bit %s\n", bool_c_str(get_m_bit()));
    output += c_format("\tMS-bit %s\n", bool_c_str(get_ms_bit()));
    output += c_format("\tDD sequence number %u", get_dd_seqno());

    list<Lsa_header> li = _lsa_headers;
    list<Lsa_header>::iterator i = li.begin();
    for (; i != li.end(); i++) {
	output += "\n\t" + (*i).str();
    }

    return output;
}

/* Link State Request Packet */

Packet *
LinkStateRequestPacket::decode(uint8_t *ptr, size_t len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateRequestPacket *packet = new LinkStateRequestPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);

    Ls_request ls(version);

    // Verify that this packet is large enough, a standard header plus
    // at least one request
    if ((len - offset) < ls.length())
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + ls.length())));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    // How many request are there?
    int requests = (len - offset) / ls.length();

    // XXX - Should we be checking for multiples of 12 here?
    for(int i = 0; i < requests; i++) {
	packet->get_ls_request().
	    push_back(ls.decode(&ptr[offset + i * ls.length()]));
    }

    return packet;
}

bool
LinkStateRequestPacket::encode(vector<uint8_t>& pkt)
{
    OspfTypes::Version version = get_version();
    Ls_request ls(version);

    size_t offset = get_standard_header_length();
    size_t len = offset + get_ls_request().size() * ls.length();

    pkt.resize(len);
    uint8_t *ptr = &pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Put the specific Link State Request Packet information first as
    // the standard header code will also add the checksum. This must
    // be done last.
    /**************************************/
    list<Ls_request> &li = get_ls_request();
    list<Ls_request>::iterator i = li.begin();
    for(size_t index = 0; i != li.end(); i++, index += ls.length()) {
	(*i).copy_out(&ptr[offset + index]);
    }

    if (offset != encode_standard_header(ptr, len)) {
	XLOG_ERROR("Encode of %s failed", str().c_str());
	return false;
    }

    return true;
}

string
LinkStateRequestPacket::str() const
{
    string output;

    output = "Link State Request Packet:\n";
    // Standard Header
    output += standard();
    // Link State Request Packet Specifics

    list<Ls_request> li = _ls_request;
    list<Ls_request>::iterator i = li.begin();
    for (; i != li.end(); i++) {
	output += "\n\t" + (*i).str();
    }

    return output;
}

/* Link State Update Packet */

Packet *
LinkStateUpdatePacket::decode(uint8_t *ptr, size_t len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateUpdatePacket *packet = new LinkStateUpdatePacket(version,
							      _lsa_decoder);

    size_t offset = packet->decode_standard_header(ptr, len);

    // Verify that this packet is large enough to hold the smallest
    // LSA that we are aware of.
    size_t min_length = _lsa_decoder.min_length();

    if ((len - offset) < min_length)
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + min_length)));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    // How many LSAs are there?
    size_t n_lsas = extract_32(&ptr[offset]);
    // Step over # LSAs
    offset += 4;

    size_t lsa_length;

    // If anything goes wrong the decoder will throw an exception.
    for(size_t i = 0; i < n_lsas; i++) {
	lsa_length = len - offset;
	packet->get_lsas().
	    push_back(_lsa_decoder.decode(&ptr[offset], lsa_length));
	offset += lsa_length;
    }

    return packet;
}

bool
LinkStateUpdatePacket::encode(vector<uint8_t>& pkt)
{
    return encode(pkt, 0 /* inftransdelay */);
}

bool
LinkStateUpdatePacket::encode(vector<uint8_t>& pkt, uint16_t inftransdelay)
{
    size_t header_offset = get_standard_header_length();
    size_t offset = header_offset;
    // Make a pass over all the LSAs to compute the total length of
    // the packet.
    size_t n_lsas = 0;
    size_t len = offset + 4;	// 4 == # LSAs

    list<Lsa::LsaRef> &lsas = get_lsas();
    list<Lsa::LsaRef>::iterator i = lsas.begin();
    for(; i != lsas.end(); i++, n_lsas++) {
	// Don't encode the LSA it should already be encoded.
	// If this is a self originating LSA then we will have encoded
	// the LSA. If we received it from a neighbour then we are not
	// supposed to mess with it apart from updating the age field.
// 	(*i)->encode();
	size_t lsa_len;
	(*i)->lsa(lsa_len);
	len += lsa_len;
    }

    pkt.resize(len);
    uint8_t *ptr = &pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Put the specific Link State Update Packet information first as
    // the standard header code will also add the checksum. This must
    // be done last.
    /**************************************/

    embed_32(&ptr[offset], n_lsas);
    offset += 4;

    for(i = lsas.begin() ; i != lsas.end(); i++) {
	size_t lsa_len;
	uint8_t *lsa_ptr;
	lsa_ptr = (*i)->lsa(lsa_len);
	memcpy(&ptr[offset], lsa_ptr, lsa_len);
	Lsa::update_age_inftransdelay(&ptr[offset], inftransdelay);
	offset += lsa_len;
    }

    if (header_offset != encode_standard_header(ptr, len)) {
	XLOG_ERROR("Encode of %s failed", str().c_str());
	return false;
    }

    return true;
}

string
LinkStateUpdatePacket::str() const
{
    string output;

    output = "Link State Update Packet:\n";
    // Standard Header
    output += standard() + "\n";
    // Link State Packet Specifics

    list<Lsa::LsaRef> li = _lsas;
    list<Lsa::LsaRef>::iterator i = li.begin();
    for (; i != li.end(); i++) {
	output += "\n\t" + (*i)->str();
    }

    return output;
}

/* Link State Acknowledgement Packet */

Packet *
LinkStateAcknowledgementPacket::decode(uint8_t *ptr, size_t len) const
    throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateAcknowledgementPacket *packet =
	new LinkStateAcknowledgementPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);

    // Verify that this packet is large enough to hold the at least
    // one LSA header.
    if ((len - offset) < Lsa_header::length())
	xorp_throw(InvalidPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + Lsa_header::length())));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    Lsa_header lsa_header(version);

    // How many LSA header are there, there should be at least 1.
    int lsas = (len - offset) / lsa_header.length();

    // XXX - Should we be checking for multiples of 20 here?
    for(int i = 0; i < lsas; i++) {
	packet->get_lsa_headers().
	    push_back(lsa_header.decode(&ptr[offset + i*lsa_header.length()]));
    }

    return packet;
}

bool
LinkStateAcknowledgementPacket::encode(vector<uint8_t>& pkt)
{
    size_t offset = get_standard_header_length();
    size_t len = offset + get_lsa_headers().size() * Lsa_header::length();

    pkt.resize(len);
    uint8_t *ptr = &pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Put the specific Link State Acknowledgement Packet information first as
    // the standard header code will also add the checksum. This must
    // be done last.

    /**************************************/
    size_t lsa_offset = offset;

    list<Lsa_header> &li = get_lsa_headers();
    list<Lsa_header>::iterator i = li.begin();
    for(size_t index = 0; i != li.end(); i++, index += Lsa_header::length()) {
	(*i).copy_out(&ptr[lsa_offset + index]);
    }

    if (offset != encode_standard_header(ptr, len)) {
	XLOG_ERROR("Encode of %s failed", str().c_str());
	return false;
    }

    return true;
}

string
LinkStateAcknowledgementPacket::str() const
{
    string output;

    output = "Link State Acknowledgement Packet:\n";
    // Standard Header
    output += standard() + "\n";
    // Link State Acknowledgement Packet Specifics

    list<Lsa_header> li = _lsa_headers;
    list<Lsa_header>::iterator i = li.begin();
    for (; i != li.end(); i++) {
	output += "\n\t" + (*i).str();
    }

    return output;
}
