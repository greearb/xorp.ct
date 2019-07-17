// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ref_ptr.hh"
#include "libxorp/timeval.hh"

#include "libproto/packet.hh"



#include "olsr_types.hh"
#include "exceptions.hh"
#include "message.hh"

// XXX: These defines don't conceptually belong here.
// We need to know the header sizes for working out how much
// space OLSR has for messages in UDP datagrams.
#define IPV4_HEADER_SIZE    20		// without any options
#define UDP_HEADER_SIZE	    8		// sizeof(udphdr)

size_t
HelloMessage::remove_link(const IPv4& remote_addr)
{
    size_t removed_count = 0;

    for (LinkBag::iterator ii = _links.begin(); ii != _links.end(); ) {
	LinkAddrInfo& lai = (*ii).second;
	if (remote_addr == lai.remote_addr()) {
	    _links.erase(ii++);
	    removed_count++;
	} else {
	    ii++;
	}
    }

    return removed_count;
}

size_t
HelloMessage::get_links_length() const
{
    if (_links.empty()) {
	return (0);
    }

    size_t byte_count = 0;

    LinkCode thislc;
    LinkBag::const_iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	const LinkAddrInfo& lai = (*ii).second;
	if (ii == _links.begin() || (*ii).first != thislc) {
	    thislc = (*ii).first;
	    if (_links.count(thislc) == 0)
		continue;
	    byte_count += link_tuple_header_length();
	}
	byte_count += lai.size();
    }

    return byte_count;
}

string
HelloMessage::str() const
{
    string str = this->common_str() + "HELLO ";
    str += "htime " + get_htime().str() + " ";
    str += "will " + c_format("%u", XORP_UINT_CAST(willingness()));

    LinkCode thislc;
    LinkBag::const_iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	if (ii == _links.begin() || (*ii).first != thislc) {
	    thislc = (*ii).first;
	    if (_links.count(thislc) == 0)
		continue;
	    str += " ";
	    str += thislc.str();
	} else {
	    str += ",";
	}
	str += " ";
	str += (*ii).second.str();
    }

    return (str += "\n");
}

size_t
HelloMessage::decode_link_tuple(uint8_t* buf, size_t& len,
    size_t& skiplen, bool haslq)
    throw(InvalidLinkTuple)
{
    size_t offset = 0;

    // minimum size check
    skiplen = len;
    if (len < link_tuple_header_length()) {
	xorp_throw(InvalidLinkTuple,
	    c_format("Runt link tuple, buffer size is %u",
		     XORP_UINT_CAST(len)));
    }

    uint8_t code = extract_8(&buf[offset]);	    // Link Code

    offset += sizeof(uint8_t);			    // Link Code
    offset += sizeof(uint8_t);			    // Reserved (skip)

    size_t link_tuple_len = extract_16(&buf[offset]);   // Link Message Size
    offset += sizeof(uint16_t);
    // We will consume the entire tuple (if possible)
    skiplen = link_tuple_len;

    bool is_bad_link_code = false;
    LinkCode linkcode;
    try {
	linkcode = code;
    } catch(BadLinkCode& blc) {
	debug_msg("caught bad link code exception\n");
	is_bad_link_code = true;
    }

    // Invalid link tuples must be discarded silently as per RFC.
    // Tell the caller to skip only the rest of the buffer, if the
    // size reported on the wire would overflow the buffer.
    if (is_bad_link_code || link_tuple_len > len) {
	if (link_tuple_len > len)
	    skiplen = len;
	xorp_throw(InvalidLinkTuple,
	    c_format("Invalid link tuple, advertised size is %u, "
		     "buffer size is %u",
		     XORP_UINT_CAST(link_tuple_len), XORP_UINT_CAST(len)));
    }

    size_t remaining = link_tuple_len - offset;
    while (remaining > 0) {
	LinkAddrInfo lai(haslq);

	if (remaining < lai.size())
	    break;

	size_t copied_in = lai.copy_in(&buf[offset]);
	offset += copied_in;
	remaining -= copied_in;

	add_link(linkcode, lai); // XXX no check for dupes
    }

    if (offset != link_tuple_len) {
	XLOG_WARNING("Link tuple has %u unparsed bytes",
		     XORP_UINT_CAST(len - offset));
    }

    skiplen = offset;

    return offset;
}

size_t
LinkAddrInfo::copy_in(const uint8_t *from_uint8)
{
    size_t offset = 0;
    const uint8_t* buf = from_uint8;

    offset += _remote_addr.copy_in(&buf[offset]);

    // Produce an encoding of the ETX measurement which is
    // compatible with olsrd.
    if (has_etx()) {
	_near_etx = extract_8(&buf[offset]);
	_near_etx /= 255;
	offset += sizeof(uint8_t);

	_far_etx = extract_8(&buf[offset]);
	_far_etx /= 255;
	offset += sizeof(uint8_t);
    }

    return offset;
}

size_t
LinkAddrInfo::copy_out(uint8_t* to_uint8) const
{
    size_t offset = 0;
    uint8_t* buf = to_uint8;

    offset += _remote_addr.copy_out(&buf[offset]);
    if (has_etx()) {
	embed_8(&buf[offset], static_cast<uint8_t>(_near_etx * 255));
	offset += sizeof(uint8_t);
	embed_8(&buf[offset], static_cast<uint8_t>(_far_etx * 255));
	offset += sizeof(uint8_t);
    }

    return offset;
}

Message*
HelloMessage::decode(uint8_t* buf, size_t& len)
    throw(InvalidMessage)
{
    if (len < min_length()) {
	xorp_throw(InvalidMessage,
	    c_format("Runt HelloMessage, size is %u",
		    XORP_UINT_CAST(len)));
    }

    HelloMessage* message = new HelloMessage();

    size_t offset = message->decode_common_header(buf, len);

    offset += sizeof(uint16_t);	    // reserved (skip)

    message->set_htime(EightBitTime::to_timeval(
	extract_8(&buf[offset])));
    offset += sizeof(uint8_t);	    // htime

    message->set_willingness(static_cast<OlsrTypes::WillType>(
	extract_8(&buf[offset])));
    offset += sizeof(uint8_t);	    // willingness

    size_t remaining = message->adv_message_length() - offset;
    while (remaining > 0) {
	size_t skiplen;
	try {
	    message->decode_link_tuple(&buf[offset], remaining, skiplen);
	} catch(InvalidLinkTuple& ilt) {
	    debug_msg("%s\n", cstring(ilt));
	    XLOG_WARNING("Invalid link info tuple at offset %u",
		    XORP_UINT_CAST(offset));
	}
	offset += skiplen;
	remaining -= skiplen;
    }

    return (message);
}

bool
HelloMessage::encode(uint8_t* buf, size_t& len)
{
    if (len < length())
	return false;

    if (!encode_common_header(buf, len))
	return false;

    size_t offset = get_common_header_length();

    embed_16(&buf[offset], 0);				// reserved
    offset += sizeof(uint16_t);

    embed_8(&buf[offset], EightBitTime::from_timeval(get_htime()));
    offset += sizeof(uint8_t);

    embed_8(&buf[offset], willingness());		// willingness
    offset += sizeof(uint8_t);

    // link tuples
    LinkCode thislc;
    for (LinkBag::iterator ii = _links.begin(); ii != _links.end(); ii++) {
	LinkAddrInfo& lai = (*ii).second;
	// Is this a new link state tuple?
	if (ii == _links.begin() || (*ii).first != thislc) {
	    thislc = (*ii).first;   // The link code of the new tuple

#ifdef DETAILED_DEBUG
	    bool haslq = lai.has_etx();

	    // Invariant: ETX measurements MUST be present in
	    // an LQ HELLO message, and MUST NOT be present in a
	    // legacy HELLO message.
	    if (0 != dynamic_cast<EtxHelloMessage*>(this)) {
		XLOG_ASSERT(haslq == true);
	    } else {
		XLOG_ASSERT(haslq == false);
	    }
#endif
	    // Don't put a link state tuple in the HELLO message if
	    // there are no neighbors for this link code.
	    size_t linksize = _links.count(thislc) * lai.size();
	    if (linksize == 0)
		continue;
	    linksize += link_tuple_header_length();

	    // Prepend the link state tuple header.
	    embed_8(&buf[offset], static_cast<uint8_t>(thislc));
	    offset += sizeof(uint8_t);
	    embed_8(&buf[offset], 0);		    // reserved
	    offset += sizeof(uint8_t);
	    embed_16(&buf[offset], linksize);	    // link message size
	    offset += sizeof(uint16_t);
	}

	// Copy out this neighbor entry.
	offset += lai.copy_out(&buf[offset]);
    }

    return true;
}

Message*
HnaMessage::decode(uint8_t* buf, size_t& len)
    throw(InvalidMessage)
{
    HnaMessage* message = new HnaMessage();

    size_t offset = message->decode_common_header(buf, len);
    size_t remaining = message->adv_message_length() - offset;

    while (remaining > 0) {
	if (remaining < (IPv4::addr_bytelen() * 2))
	    break;

	IPv4 addr(&buf[offset]);
	offset += addr.addr_bytelen();
	remaining -= addr.addr_bytelen();

	IPv4 mask(&buf[offset]);
	offset += mask.addr_bytelen();
	remaining -= mask.addr_bytelen();

	uint32_t prefix_len = mask.mask_len();
	addr.mask_by_prefix_len(prefix_len);

	message->add_network(IPv4Net(addr, prefix_len));
    }

    if (message->networks().empty()) {
	xorp_throw(InvalidMessage,
	    c_format("Runt HnaMessage, size is %u",
		    XORP_UINT_CAST(len)));
    }

    return (message);
}

bool
HnaMessage::encode(uint8_t* buf, size_t& len)
{
    if (len < length())
	return false;

    if (!encode_common_header(buf, len))
	return false;

    size_t offset = get_common_header_length();

    vector<IPv4Net>::const_iterator ii;
    for (ii = _networks.begin(); ii != _networks.end(); ii++) {
	offset += (*ii).masked_addr().copy_out(&buf[offset]);
	offset += (*ii).netmask().copy_out(&buf[offset]);
    }

    return true;
}

string
Message::common_str() const
{
    string s = c_format(
	"msg: type %d vtime %s size %u origin %s ttl %u hops %u seq %u\n",
	XORP_UINT_CAST(type()),
	cstring(expiry_time()),
	XORP_UINT_CAST(length()),
	cstring(origin()),
	XORP_UINT_CAST(ttl()),
	XORP_UINT_CAST(hops()),
	XORP_UINT_CAST(seqno()));

    return (s);
}

size_t
Message::decode_common_header(uint8_t* ptr, size_t& len)
    throw(InvalidMessage)
{

    if (len < Message::get_common_header_length()) {
	xorp_throw(InvalidPacket,
	    c_format("Message too short %u, must be at least %u",
		    XORP_UINT_CAST(len),
		    XORP_UINT_CAST(Message::get_common_header_length())));
    }

    _adv_message_length = extract_16(&ptr[2]);
    if (_adv_message_length > len) {
	xorp_throw(InvalidMessage,
	    c_format("Message too short %u, advertised size is %u",
		XORP_UINT_CAST(len),
		XORP_UINT_CAST(_adv_message_length)));
    }

    set_type(ptr[0]);
    _expiry_time = EightBitTime::to_timeval(ptr[1]);
    _msg.resize(extract_16(&ptr[2]));

    // XXX address family; we should use a pointer of our own for
    // dealing with IPv6 when refactoring.
    _origin.copy_in(&ptr[4]);

    // Do not perform the ttl check here. We do that elsewhere, i.e.
    // when sorting the messages into an input queue.
    set_ttl(ptr[8]);
    set_hop_count(ptr[9]);
    set_seqno(extract_16(&ptr[10]));

    // 3.4, 2: Drop messages with invalid TTL.
    if (ttl() <= 0) {
	xorp_throw(InvalidMessage,
	    c_format("Invalid message TTL %u.", XORP_UINT_CAST(ttl())));
    }

    // Store the entire message in the Message parent class _msg field.
    store(ptr, _adv_message_length);

    set_valid(true);	    // XXX should we do that here?

    return (Message::get_common_header_length());
}

bool
Message::encode_common_header(uint8_t* ptr, size_t& len)
{
    if (len < Message::get_common_header_length()) {
	debug_msg("insufficient space for common message header");
	return false;
    }

    embed_8(&ptr[0], type());	// message type
    uint8_t vtime = EightBitTime::from_timeval(expiry_time());
    embed_8(&ptr[1], vtime);	// valid/expiry time
    embed_16(&ptr[2], length());	// message length
    origin().copy_out(&ptr[4]);
    embed_8(&ptr[8], ttl());	// ttl
    embed_8(&ptr[9], hops());		// hop count
    embed_16(&ptr[10], seqno());	// message sequence number

    return true;
}

MessageDecoder::~MessageDecoder()
{
    map<OlsrTypes::MessageType, Message* >::iterator i;

    for (i = _olsrv1.begin(); i != _olsrv1.end(); i++)
	delete i->second;
}

Message*
MessageDecoder::decode(uint8_t* ptr, size_t len)
    throw(InvalidMessage)
{

    // Message knows what the minimum size is -- but only for its
    // address family.

    if (len < Message::get_common_header_length()) {
	xorp_throw(InvalidMessage,
	    c_format("Message too short %u, must be at least %u",
		    XORP_UINT_CAST(len),
		    XORP_UINT_CAST(Message::get_common_header_length())));
    }

    // OLSRv1 type code offset; unchanged by address family.
    uint8_t type = ptr[0];

    Message* decoder;
    map<OlsrTypes::MessageType, Message*>::iterator ii = _olsrv1.find(type);
    if (ii == _olsrv1.end()) {
	decoder = &_olsrv1_unknown;
    } else {
	decoder = (*ii).second;
    }

    return (decoder->decode(ptr, len));
}

void
MessageDecoder::register_decoder(Message* message)
{
    XLOG_ASSERT(_olsrv1.find(message->type()) == _olsrv1.end());
    XLOG_ASSERT(0 != message->type());
    _olsrv1[message->type()] = message;
}

Message*
MidMessage::decode(uint8_t* buf, size_t& len)
    throw(InvalidMessage)
{
    MidMessage* message = new MidMessage();

    size_t offset = message->decode_common_header(buf, len);
    size_t remaining = message->adv_message_length() - offset;

    while (remaining > 0) {
	if (remaining < IPv4::addr_bytelen())
	    break;
	message->add_interface(IPv4(&buf[offset]));
	offset += IPv4::addr_bytelen();
	remaining -= IPv4::addr_bytelen();
    }

    if (message->interfaces().empty()) {
	xorp_throw(InvalidMessage,
	    c_format("Runt MidMessage, size is %u",
		    XORP_UINT_CAST(len)));
    }

    return (message);
}

bool
MidMessage::encode(uint8_t* buf, size_t& len)
{
    if (len < length())
	return false;

    if (!encode_common_header(buf, len))
	return false;

    size_t offset = get_common_header_length();

    vector<IPv4>::const_iterator ii;
    for (ii = _interfaces.begin(); ii != _interfaces.end(); ii++)
	offset += (*ii).copy_out(&buf[offset]);

    return true;
}

void
Packet::decode(uint8_t* ptr, size_t len)
    throw(InvalidPacket)
{
    size_t offset = decode_packet_header(ptr, len);
    size_t remaining = len - offset;
    size_t index = 0;

    while (remaining > 0) {
	Message* message;
	try {
	    message = _message_decoder.decode(&ptr[offset], len - offset);

	    message->set_is_first(0 == index++);
	    message->set_faceid(faceid());

	    offset += message->length();
	    remaining -= message->length();

	    add_message(message);
	} catch (InvalidMessage& e) {
	    debug_msg("%s\n", cstring(e));
	    break;
	}
    }

   // 3.4, 1: Packet Processing: A node must discard a message with
   // no messages.
    if (_messages.empty()) {
	xorp_throw(InvalidPacket, c_format("Packet contains no messages."));
    }

    // Mark the last message in the packet for use with future S-OLSR
    // decoders as they need to track this information.
    (*_messages.rbegin())->set_is_last(true);

    if (remaining > 0) {
	debug_msg("Packet has %d bytes remaining\n", XORP_INT_CAST(remaining));
    }
}

size_t
Packet::decode_packet_header(uint8_t* ptr, size_t len)
    throw(InvalidPacket)
{
    // 3.4, 1: Packet Processing: A node must discard a message with
    // no messages.
    if (len <= Packet::get_packet_header_length()) {
	xorp_throw(InvalidPacket,
	    c_format("Packet too short %u, must be > %u",
		    XORP_UINT_CAST(len),
		    XORP_UINT_CAST(Packet::get_packet_header_length())));
    }

    size_t packet_length = extract_16(&ptr[0]);
    if (len < packet_length) {
	xorp_throw(InvalidPacket,
	    c_format("Packet too short %u, advertised size is %u",
		    XORP_UINT_CAST(len),
		    XORP_UINT_CAST(packet_length)));
    }

    store(ptr, packet_length);
    set_seqno(extract_16(&ptr[2]));

    return (get_packet_header_length());
}

// without mtu
size_t
Packet::length() const
{
    size_t len = get_packet_header_length();

    vector<Message*>::const_iterator ii;
    for (ii = _messages.begin(); ii != _messages.end(); ii++)
	len += (*ii)->length();

    return len;
}

size_t
Packet::mtu_bound() const
{
    return mtu() - IPV4_HEADER_SIZE - UDP_HEADER_SIZE;
};

// bound by mtu
size_t
Packet::bounded_length() const
{
    if (mtu() == 0)
	return length();

    // maximum OLSR packet length
    size_t free_bytes = mtu_bound();
    size_t len = get_packet_header_length();

    //debug_msg("mtu %u free_bytes %u\n",
    //	      XORP_UINT_CAST(mtu()),
    //	      XORP_UINT_CAST(free_bytes));

    vector<Message*>::const_iterator ii;
    for (ii = _messages.begin(); ii != _messages.end(); ii++) {
	//debug_msg("about to take that little step\n");
	size_t msglen = (*ii)->length();
	// Check if appending this message would overflow MTU.
	if (len + msglen > free_bytes)
	    break;
	len += msglen;
    }

    return len;
}

bool
Packet::encode(vector<uint8_t>& pkt)
{
    size_t bounded_len = bounded_length();
    pkt.resize(bounded_len);

    uint8_t *ptr = &pkt[0];
    memset(ptr, 0, bounded_len);

    embed_16(&ptr[0], bounded_len);
    embed_16(&ptr[2], seqno());

    size_t offset = get_packet_header_length();

    vector<Message*>::const_iterator ii;
    for (ii = _messages.begin(); ii != _messages.end(); ii++) {
	size_t msglen = (*ii)->length();
	// Check if appending this message would overflow MTU.
	if (offset + msglen > bounded_len) {
	    debug_msg("mtu overflowed: %u + %u > %u",
		      XORP_UINT_CAST(offset),
		      XORP_UINT_CAST(msglen),
		      XORP_UINT_CAST(bounded_len));
	    return false;
	}
	// Try to encode this message.
	if (false == (*ii)->encode(&pkt[offset], msglen))
	    return false;
	offset += msglen;
    }

    return true;
}

void
Packet::update_encoded_seqno(vector<uint8_t>& pkt)
{
    embed_16(&pkt[2], seqno());
}

string
Packet::str() const
{
    string s = c_format("OLSRv1: len %u seq %u\n",
	XORP_UINT_CAST(length()),
	XORP_UINT_CAST(seqno()));
    vector<Message*>::const_iterator ii;
    for (ii = _messages.begin(); ii != _messages.end(); ii++)
	s += (*ii)->str();
    return (s += '\n');
}

void
TcMessage::decode_tc_common(uint8_t* buf, size_t& len, bool haslq)
    throw(InvalidMessage)
{
    size_t offset = decode_common_header(buf, len);
    size_t remaining = adv_message_length() - min_length();

    set_ansn(extract_16(&buf[offset]));
    offset += sizeof(uint16_t);		// ANSN
    offset += sizeof(uint16_t);		// Reserved

    while (remaining > 0) {
	LinkAddrInfo lai(haslq);
	if (remaining < lai.size())
	    break;

	size_t copied_in = lai.copy_in(&buf[offset]);
	offset += copied_in;
	remaining -= copied_in;

	add_neighbor(lai);
    }
}

Message*
TcMessage::decode(uint8_t* buf, size_t& len)
    throw(InvalidMessage)
{
    if (len < min_length()) {
	xorp_throw(InvalidMessage,
	    c_format("Runt TcMessage, size is %u", XORP_UINT_CAST(len)));
    }

    TcMessage* message = new TcMessage();
    try {
	message->decode_tc_common(buf, len);
    } catch (...) {
	throw;
    }

    return (message);
}

Message*
EtxTcMessage::decode(uint8_t* buf, size_t& len)
    throw(InvalidMessage)
{
    if (len < min_length()) {
	xorp_throw(InvalidMessage,
	    c_format("Runt EtxTcMessage, size is %u", XORP_UINT_CAST(len)));
    }

    EtxTcMessage* message = new EtxTcMessage();
    try {
	message->decode_tc_common(buf, len, true);
    } catch (...) {
	throw;
    }

    return (message);
}

bool
TcMessage::encode(uint8_t* buf, size_t& len)
{
    if (len < length())
	return false;

    if (!encode_common_header(buf, len))
	return false;

    size_t offset = get_common_header_length();

    embed_16(&buf[offset], ansn());
    offset += sizeof(uint16_t);
    embed_16(&buf[offset], 0);
    offset += sizeof(uint16_t);

    vector<LinkAddrInfo>::const_iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	const LinkAddrInfo& lai = *ii;
#ifdef DETAILED_DEBUG
	// Invariant: ETX information MUST be present in
	// an LQ TC message, and MUST NOT be present in a legacy TC message.
	if (0 != dynamic_cast<EtxTcMessage*>(this)) {
	    XLOG_ASSERT(lai.has_etx() == true);
	} else {
	    XLOG_ASSERT(lai.has_etx() == false);
	}
#endif
	offset += lai.copy_out(&buf[offset]);
    }

    return true;
}

Message*
UnknownMessage::decode(uint8_t* ptr, size_t& len)
    throw(InvalidMessage)
{
    UnknownMessage* message = new UnknownMessage();

    size_t offset = message->decode_common_header(ptr, len);

    UNUSED(offset);

    return (message);
}

bool
UnknownMessage::encode(uint8_t* ptr, size_t& len)
{
    store(ptr, len);

    return true;
}

string
MidMessage::str() const
{
    string str = this->common_str() + "MID ";
    if (_interfaces.empty()) {
	str = "<empty>";
    } else {
	vector<IPv4>::const_iterator ii;
	for (ii = _interfaces.begin(); ii != _interfaces.end(); ii++)
	str += ii->str() + " ";
    }
    return (str += "\n");
}

#define NAME_CASE(x) case OlsrTypes:: x : return x##_NAME ;
#define NUM_CASE(x) case x : return #x ;

static const char* UNSPEC_LINK_NAME = "unspec";
static const char* ASYM_LINK_NAME = "asym";
static const char* SYM_LINK_NAME = "sym";
static const char* LOST_LINK_NAME = "lost";

const char*
LinkCode::linktype_to_str(OlsrTypes::LinkType t)
{
    switch (t) {
	NAME_CASE(UNSPEC_LINK);
	NAME_CASE(ASYM_LINK);
	NAME_CASE(SYM_LINK);
	NAME_CASE(LOST_LINK);
    }
    XLOG_UNREACHABLE();
    return 0;
}

static const char* NOT_NEIGH_NAME = "not";
static const char* SYM_NEIGH_NAME = "sym";
static const char* MPR_NEIGH_NAME = "mpr";

const char*
LinkCode::neighbortype_to_str(OlsrTypes::NeighborType t)
{
    switch (t) {
	NAME_CASE(NOT_NEIGH);
	NAME_CASE(SYM_NEIGH);
	NAME_CASE(MPR_NEIGH);
    }
    XLOG_UNREACHABLE();
    return 0;
}

#undef NUM_CASE
#undef NAME_CASE
