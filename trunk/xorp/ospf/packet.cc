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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
// #define DEBUG_RAW_PACKETS

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <vector>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "packet.hh"

/**
 * XXX - Might make sense to move this into libxorp.
 *     - Rewrite this routine to not perform the intermediate copy and
 *       keep it portable to architectures that don't allow
 *       non-aligned accesses.
 *
 * Compute the IP checksum.
 */
inline
uint16_t
checksum(uint8_t *ptr, size_t len)
{
    // Copy the buffer to be checksumed into a temporary buffer to
    // deal with alignment and odd number of bytes issues.
    size_t templen = (len + 1) / 2;
    uint16_t temp[templen];
    temp[templen - 1] = 0;// Put a zero at the end if the buffer to
			  // deal with an odd number of bytes.

    memcpy(&temp[0], ptr, len);

    uint32_t sum = 0;
    for(size_t i = 0; i < templen; i++) {
// 	printf("%u %#x\n", i * 2, temp[i]);
	sum += temp[i];
    }

    // Fold the carry back in.
    sum += (sum >> 16) & 0xffff;

    uint16_t result = sum;
    result ^= 0xffff;

    return result;
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
Packet::decode_standard_header(uint8_t *ptr, size_t len) throw(BadPacket)
{
    debug_msg("ptr %p len %u\n", ptr, XORP_UINT_CAST(len));
#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    // Make sure that at least two bytes have been extracted:
    // Version and Type fields.
    if (len < 2)
	xorp_throw(BadPacket,
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
	xorp_throw(BadPacket,
		   c_format("Version mismatch expected %u received %u",
			    get_version(),
			    ptr[0] & 0xff));
	break;
    }

    if (ptr[1] != get_type())
	xorp_throw(BadPacket,
		   c_format("Type mismatch expected %u received %u",
			    get_type(),
			    ptr[1]));

    // Make sure that at least the standard header length is present.
    switch(version) {
    case OspfTypes::V2:
	if (len < STANDARD_HEADER_V2)
	    xorp_throw(BadPacket,
		       c_format("Packet too short %u, must be at least %u",
				XORP_UINT_CAST(len),
				XORP_UINT_CAST(STANDARD_HEADER_V2)));
    case OspfTypes::V3:
	if (len < STANDARD_HEADER_V3)
	    xorp_throw(BadPacket,
		       c_format("Packet too short %u, must be at least %u",
				XORP_UINT_CAST(len),
				XORP_UINT_CAST(STANDARD_HEADER_V3)));
    }

    // Verify that the length in the packet and the length of received
    // data match.
    uint32_t packet_length = extract_16(&ptr[2]);
    if (packet_length != len)
	xorp_throw(BadPacket,
		   c_format("Packet length mismatch expected %u received %u",
			    packet_length,
			    XORP_UINT_CAST(len)));

    set_router_id(IPv4(&ptr[4]));
    set_area_id(IPv4(&ptr[8]));

    // In OSPFv2 there is authentication info in the standard header.
    switch(version) {
    case OspfTypes::V2:
	// Verify the auth structure is the correct size.
	static_assert(sizeof(_auth) == (64 / 8));
	set_auth_type(extract_16(&ptr[14]));
	memcpy(&_auth[0], &ptr[16], sizeof(_auth));
	// The authentication field is expected to be zero for the
	// checksumming.
	memset(&ptr[16], 0, sizeof(_auth));
	break;
    case OspfTypes::V3:
	set_instance_id(ptr[14]);
	break;
    }


    // Extract the checksum and check the packet.
    uint16_t checksum_inpacket = extract_16(&ptr[12]);
    // Zero the checksum location.
    embed_16(&ptr[12], 0);
    uint16_t checksum_actual = checksum(ptr, len);

    // Restore the zero'd fields.
    switch(version) {
    case OspfTypes::V2:
	memcpy(&ptr[16], &_auth[0], sizeof(_auth));
	break;
    case OspfTypes::V3:
	break;
    }
    embed_16(&ptr[12], checksum_inpacket);
    
    if (checksum_inpacket != checksum_actual)
	xorp_throw(BadPacket,
		   c_format("Checksum mismatch expected %#x received %#x",
			    checksum_inpacket,
			    checksum_actual));

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

    ptr[0] = version;
    ptr[1] = get_type();
    embed_16(&ptr[2], len);
    get_router_id().copy_out(&ptr[4]);
    get_area_id().copy_out(&ptr[8]);
    
    switch(version) {
    case OspfTypes::V2:
	embed_16(&ptr[14], get_auth_type());
	break;
    case OspfTypes::V3:
	ptr[14] = get_instance_id();
	break;
    }

    embed_16(&ptr[12], checksum(ptr, len));

    // OSPF V2 only copy the authentication data out.
    switch(version) {
    case OspfTypes::V2:
	memcpy(&ptr[16], &_auth[0], sizeof(_auth));
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
    output += "\tRouter ID " + get_router_id().str() + "\n";
    output += "\tArea ID " + get_area_id().str() + "\n";

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
	_ospfv2[packet->get_type()] = packet;
	break;
    case OspfTypes::V3:
	_ospfv3[packet->get_type()] = packet;
	break;
    }
}

Packet *
PacketDecoder::decode(uint8_t *ptr, size_t len) throw(BadPacket)
{
    // Make sure that at least two bytes have been extracted:
    // Version and Type fields.
    if (len < 2)
	xorp_throw(BadPacket,
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
	xorp_throw(BadPacket,
		   c_format("Unknown OSPF Version %u", ptr[0] & 0xff));
	break;
    }

    map<OspfTypes::Type , Packet *>::iterator i;
    uint8_t type = ptr[1];
    bool bad = false;
    switch(version) {
    case OspfTypes::V2:
	i = _ospfv2.find(type);
	if (i == _ospfv2.end())
	    bad = true;
	break;
    case OspfTypes::V3:
	i = _ospfv3.find(type);
	if (i == _ospfv3.end())
	    bad = true;
	break;
    }
    
    if (bad)
	xorp_throw(BadPacket,
		   c_format("OSPF Version %u Unknown Type %u", version, type));
    
    Packet *packet = i->second;

    return packet->decode(ptr, len);
}

/* HelloPacket */

Packet *
HelloPacket::decode(uint8_t *ptr, size_t len) const throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    HelloPacket *packet = new HelloPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);
    
    // Verify that this packet is large enough, up to but not including
    // any neighbours.
    if ((len - offset) < MINIMUM_LENGTH)
	xorp_throw(BadPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + MINIMUM_LENGTH)));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    switch(version) {
    case OspfTypes::V2:
	packet->set_network_mask(extract_32(&ptr[offset]));
	packet->set_hello_interval(extract_16(&ptr[offset + 4]));
	packet->set_options(ptr[offset + 6]);
	packet->set_router_priority(ptr[offset + 7]);
	packet->set_router_dead_interval(extract_32(&ptr[offset + 8]));
	break;
    case OspfTypes::V3:
	packet->set_interface_id(extract_32(&ptr[offset]));
	packet->set_router_priority(ptr[offset + 4]);
	packet->set_options(extract_32(&ptr[offset + 4]) & 0xffffff);
	packet->set_hello_interval(extract_16(&ptr[offset + 8]));
	packet->set_router_dead_interval(extract_16(&ptr[offset + 10]));
	break;
    }

    packet->set_designated_router(IPv4(&ptr[offset + 12]));
    packet->set_backup_designated_router(IPv4(&ptr[offset + 16]));

    // If there is any more space in the packet extract the neighbours.
    int neighbours = (len - (offset + MINIMUM_LENGTH)) / 4;

    // XXX - Should we be checking for multiples of 4 here?

    for(int i = 0; i < neighbours; i++)
	packet->get_neighbours().
	    push_back(IPv4(&ptr[offset + MINIMUM_LENGTH + i*4]));

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
	embed_32(&ptr[offset], get_network_mask());
	embed_16(&ptr[offset + 4], get_hello_interval());
	ptr[offset + 6] = get_options();
	ptr[offset + 7] = get_router_priority();
	embed_32(&ptr[offset +8], get_router_dead_interval());
	break;
    case OspfTypes::V3:
	embed_32(&ptr[offset], get_interface_id());
	// Careful Options occupy 3 bytes, four bytes are written out
	// and the top byte is overwritten by the router priority.
	embed_32(&ptr[offset + 4], get_options());
	ptr[offset + 4] = get_router_priority();

	embed_16(&ptr[offset + 8], get_hello_interval());
	embed_16(&ptr[offset + 10], get_router_dead_interval());
	break;
    }

    get_designated_router().copy_out(&ptr[offset + 12]);
    get_backup_designated_router().copy_out(&ptr[offset + 16]);

    list<OspfTypes::RouterID> &li = get_neighbours();
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(size_t index = 0; i != li.end(); i++, index += 4) {
	(*i).copy_out(&ptr[offset + 20 + index]);
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
    output += "\tDesignated Router " + get_designated_router().str() + "\n";
    output += "\tBackup Designated Router " +
	get_backup_designated_router().str();

    list<OspfTypes::RouterID> li = _neighbours;
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(; i != li.end(); i++) {
	output += "\n\tNeighbour: " + (*i).str();
    }
    
    return output;
}


/* Database Description packet */

Packet *
DataDescriptionPacket::decode(uint8_t *ptr, size_t len) const throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    DataDescriptionPacket *packet = new DataDescriptionPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);
    
    // Verify that this packet is large enough, up to but not including
    // any neighbours.
    if ((len - offset) < minimum_length())
	xorp_throw(BadPacket,
		   c_format("Packet too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(offset + minimum_length())));

#ifdef	DEBUG_RAW_PACKETS
    debug_msg("\n%s", dump_packet(ptr, len).c_str());
#endif

    size_t bias;

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

    size_t bias;

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
    output += c_format("\tI-bit %s\n", get_i_bit() ? "true" : "false");
    output += c_format("\tM-bit %s\n", get_m_bit() ? "true" : "false");
    output += c_format("\tMS-bit %s\n", get_ms_bit() ? "true" : "false");
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
LinkStateRequestPacket::decode(uint8_t *ptr, size_t len) const throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateRequestPacket *packet = new LinkStateRequestPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);
    
    Ls_request ls(version);

    // Verify that this packet is large enough, a standard header plus
    // at least one request
    if ((len - offset) < ls.length())
	xorp_throw(BadPacket,
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
LinkStateUpdatePacket::decode(uint8_t *ptr, size_t len) const throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateUpdatePacket *packet = new LinkStateUpdatePacket(version,
							      _lsa_decoder);

    size_t offset = packet->decode_standard_header(ptr, len);
    
    // Verify that this packet is large enough to hold the smallest
    // LSA that we are aware of.
    size_t min_length = _lsa_decoder.min_length();

    if ((len - offset) < min_length)
	xorp_throw(BadPacket,
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
    throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    LinkStateAcknowledgementPacket *packet = 
	new LinkStateAcknowledgementPacket(version);

    size_t offset = packet->decode_standard_header(ptr, len);
    
    // Verify that this packet is large enough to hold the at least
    // one LSA header.
    if ((len - offset) < Lsa_header::length())
	xorp_throw(BadPacket,
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
