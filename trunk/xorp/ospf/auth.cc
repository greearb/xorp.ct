// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/auth.cc,v 1.6 2005/12/28 18:57:17 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "auth.hh"

/**
 * RFC 1141 Incremental Updating of the Internet Checksum
 */
inline
uint16_t
incremental_checksum(uint32_t orig, uint32_t add)
{
    // Assumes that the original value of the modified field was zero.
    uint32_t sum = orig + (~add & 0xffff);
    sum = (sum & 0xffff) + (sum >> 16);
    return sum;
}

/**
 * Create a printable string of an array of data
 */
inline
string
printable(const uint8_t* p, size_t max_size)
{
    string output;

    for (size_t i = 0; i < max_size; i++, p++) {
	if ('\0' == *p)
	    break;
	if (xorp_isprint(*p))
	    output += *p;
	else
	    output += c_format("[%#x]", *p);
    }
    
    return output;
}

// ----------------------------------------------------------------------------
// AuthHandlerBase implementation

AuthHandlerBase::~AuthHandlerBase()
{
}

const string&
AuthHandlerBase::error() const
{
    return _error;
}

inline void
AuthHandlerBase::reset_error()
{
    if (_error.empty() == false)
	_error.erase();
}

inline void
AuthHandlerBase::set_error(const string& error_msg)
{
    _error = error_msg;
}

// ----------------------------------------------------------------------------
// NullAuthHandler implementation

const char*
NullAuthHandler::name() const
{
    return auth_type_name();
}

const char*
NullAuthHandler::auth_type_name()
{
    return "none";
}

bool
NullAuthHandler::authenticate_inbound(const vector<uint8_t>& pkt,
				      const IPv4&,
				      bool)
{
    // TODO: here we should check whether the packet is too large
    if (pkt.size() < Packet::STANDARD_HEADER_V2) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(pkt.size())));
	return false;
    }

    const uint8_t *ptr = &pkt[0];
    OspfTypes::AuType autype = extract_16(&ptr[Packet::AUTH_TYPE_OFFSET]);
    if (AUTH_TYPE != autype) {
	set_error(c_format("unexpected authentication data (type %d)",
			   autype));
	return false;
    }

    reset_error();
    return true;
}

bool
NullAuthHandler::authenticate_outbound(vector<uint8_t>& pkt)
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET],	// RFC 1141
	     incremental_checksum(extract_16(&ptr[Packet::CHECKSUM_OFFSET]),
				  AUTH_TYPE));

    reset_error();
    return (true);
}

void
NullAuthHandler::reset()
{
}

// ----------------------------------------------------------------------------
// Plaintext handler implementation

const char*
PlaintextAuthHandler::name() const
{
    return auth_type_name();
}

const char*
PlaintextAuthHandler::auth_type_name()
{
    return "simple";
}

void
PlaintextAuthHandler::set_key(const string& plaintext_key)
{
    _key = string(plaintext_key, 0, Packet::AUTH_PAYLOAD_SIZE);
    memset(&_key_data[0], 0, sizeof(_key_data));
    size_t len = _key.size();
    if (len > sizeof(_key_data))
	len = sizeof(_key_data);
    memcpy(&_key_data[0], _key.c_str(), len);
}

const string&
PlaintextAuthHandler::key() const
{
    return _key;
}

bool
PlaintextAuthHandler::authenticate_inbound(const vector<uint8_t>& pkt,
					   const IPv4&,
					   bool)
{
    // TODO: here we should check whether the packet is too large
    if (pkt.size() < Packet::STANDARD_HEADER_V2) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(pkt.size())));
	return false;
    }

    const uint8_t *ptr = &pkt[0];
    OspfTypes::AuType autype = extract_16(&ptr[Packet::AUTH_TYPE_OFFSET]);
    if (AUTH_TYPE != autype) {
	set_error("not a plaintext authenticated packet");
	return false;
    }

    if (0 != memcmp(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_key_data[0], 
		    sizeof(_key_data))) {
	string passwd = printable(&ptr[Packet::AUTH_PAYLOAD_OFFSET],
				  Packet::AUTH_PAYLOAD_SIZE);
	set_error(c_format("wrong password \"%s\"", passwd.c_str()));
	return false;
    }

    reset_error();
    return true;
}

bool
PlaintextAuthHandler::authenticate_outbound(vector<uint8_t>& pkt)
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET], // RFC 1141
	     incremental_checksum(extract_16(&ptr[Packet::CHECKSUM_OFFSET]),
				  AUTH_TYPE));

    memcpy(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_key_data[0],
	   sizeof(_key_data));

    reset_error();
    return (true);
}

void
PlaintextAuthHandler::reset()
{
}

// ----------------------------------------------------------------------------
// MD5AuthHandler::MD5Key implementation

MD5AuthHandler::MD5Key::MD5Key(uint8_t		key_id,
			       const string&	key,
			       uint32_t		start_secs,
			       XorpTimer	to)
    : _id(key_id), _start_secs(start_secs), _o_seqno(0), _to(to)
{
    string::size_type n = key.copy(_key_data, 16);
    if (n < KEY_BYTES) {
	memset(_key_data + n, 0, KEY_BYTES - n);
    }
}

string
MD5AuthHandler::MD5Key::key() const
{
    return string(_key_data, 0, 16);
}

uint32_t
MD5AuthHandler::MD5Key::end_secs() const
{
    if (_to.scheduled()) {
	return _to.expiry().sec();
    }
    return _start_secs;
}

bool
MD5AuthHandler::MD5Key::persistent() const
{
    return _to.scheduled() == false;
}

bool
MD5AuthHandler::MD5Key::valid_at(uint32_t when_secs) const
{
    if (persistent())
	return true;

    // XXX Need to deal with clock wrap.
    if (when_secs - start_secs() > 0x7fffffff)
	return false;

    // Subtract start time to partially mitigate problems of clock wrap.
    return (when_secs - start_secs() <= end_secs() - start_secs());
}

void
MD5AuthHandler::MD5Key::reset()
{
    //
    // Reset the seqno
    //
    _lr_seqno.clear();

    //
    // Reset the flag that a packet has been received
    //
    _pkts_recv.clear();
}

void
MD5AuthHandler::MD5Key::reset(const IPv4& src_addr)
{
    map<IPv4, uint32_t>::iterator seqno_iter;
    map<IPv4, bool>::iterator recv_iter;

    //
    // Reset the seqno
    //
    seqno_iter = _lr_seqno.find(src_addr);
    if (seqno_iter != _lr_seqno.end())
	_lr_seqno.erase(seqno_iter);

    //
    // Reset the flag that a packet has been received
    //
    recv_iter = _pkts_recv.find(src_addr);
    if (recv_iter != _pkts_recv.end())
	_pkts_recv.erase(recv_iter);
}

bool
MD5AuthHandler::MD5Key::packets_received(const IPv4& src_addr) const
{
    map<IPv4, bool>::const_iterator iter;

    iter = _pkts_recv.find(src_addr);
    if (iter == _pkts_recv.end())
	return (false);

    return (iter->second);
}

uint32_t
MD5AuthHandler::MD5Key::last_seqno_recv(const IPv4& src_addr) const
{
    map<IPv4, uint32_t>::const_iterator iter;

    iter = _lr_seqno.find(src_addr);
    if (iter == _lr_seqno.end())
	return (0);

    return (iter->second);
}

void
MD5AuthHandler::MD5Key::set_last_seqno_recv(const IPv4& src_addr,
					    uint32_t seqno)
{
    map<IPv4, uint32_t>::iterator seqno_iter;
    map<IPv4, bool>::iterator recv_iter;

    //
    // Set the seqno
    //
    seqno_iter = _lr_seqno.find(src_addr);
    if (seqno_iter == _lr_seqno.end())
	_lr_seqno.insert(make_pair(src_addr, seqno));
    else
	seqno_iter->second = seqno;

    //
    // Set the flag that a packet has been received
    //
    recv_iter = _pkts_recv.find(src_addr);
    if (recv_iter == _pkts_recv.end())
	_pkts_recv.insert(make_pair(src_addr, true));
    else
	recv_iter->second = true;
}

// ----------------------------------------------------------------------------
// MD5AuthHandler implementation

MD5AuthHandler::MD5AuthHandler(EventLoop& eventloop)
    : _eventloop(eventloop)
{
}

const char*
MD5AuthHandler::name() const
{
    return auth_type_name();
}

const char*
MD5AuthHandler::auth_type_name()
{
    return "md5";
}

bool
MD5AuthHandler::add_key(uint8_t       key_id,
			const string& key,
			uint32_t      start_secs,
			uint32_t      end_secs)
{
    if (start_secs > end_secs) {
	return false;
    }

    XorpTimer timeout;
    if (start_secs != end_secs) {
	timeout = _eventloop.new_oneoff_at(TimeVal(end_secs, 0),
					   callback(this,
						    &MD5AuthHandler::remove_key_cb, key_id));
    }

    KeyChain::iterator i = _key_chain.begin();
    while (i != _key_chain.end()) {
	if (key_id == i->id()) {
	    *i = MD5Key(key_id, key, start_secs, timeout);
	    return true;
	}
	++i;
    }
    _key_chain.push_back(MD5Key(key_id, key, start_secs, timeout));
    return true;
}

bool
MD5AuthHandler::remove_key(uint8_t key_id)
{
    KeyChain::iterator i =
	find_if(_key_chain.begin(), _key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (_key_chain.end() != i) {
	_key_chain.erase(i);
	return true;
    }
    return false;
}

MD5AuthHandler::KeyChain::const_iterator
MD5AuthHandler::key_at(uint32_t now_secs) const
{
    return find_if(_key_chain.begin(), _key_chain.end(),
		   bind2nd(mem_fun_ref(&MD5Key::valid_at), now_secs));
}

MD5AuthHandler::KeyChain::iterator
MD5AuthHandler::key_at(uint32_t now_secs)
{
    return find_if(_key_chain.begin(), _key_chain.end(),
		   bind2nd(mem_fun_ref(&MD5Key::valid_at), now_secs));
}

uint16_t
MD5AuthHandler::currently_active_key() const
{
    TimeVal now;
    _eventloop.current_time(now);
    KeyChain::const_iterator ki = key_at(now.sec());
    if (_key_chain.end() != ki) {
	return ki->id();
    }
    return 0xffff;	// an invalid key ID
}

void
MD5AuthHandler::reset_keys()
{
    KeyChain::iterator iter;

    for (iter = _key_chain.begin(); iter != _key_chain.end(); ++iter)
	iter->reset();
}

bool
MD5AuthHandler::authenticate_inbound(const vector<uint8_t>& pkt,
				     const IPv4& src_addr, bool new_peer)
{
    // TODO: here we should check whether the packet is too large
    if (pkt.size() < Packet::STANDARD_HEADER_V2) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(pkt.size())));
	return false;
    }

    const uint8_t *ptr = &pkt[0];
    OspfTypes::AuType autype = extract_16(&ptr[Packet::AUTH_TYPE_OFFSET]);
    if (AUTH_TYPE != autype) {
	set_error("not an MD5 authenticated packet");
	return false;
    }

    uint8_t key_id = ptr[Packet::AUTH_PAYLOAD_OFFSET + 2];
    uint32_t seqno = extract_32(&ptr[Packet::AUTH_PAYLOAD_OFFSET + 4]);

    KeyChain::iterator k = find_if(_key_chain.begin(), _key_chain.end(),
				   bind2nd(mem_fun_ref(&MD5Key::id_matches),
					   key_id));
    if (_key_chain.end() == k) {
	set_error(c_format("packet with key ID %d for which no key is "
			   "configured", key_id));
	return false;
    }

    if (new_peer)
	k->reset(src_addr);

    uint32_t last_seqno_recv = k->last_seqno_recv(src_addr);
    if (k->packets_received(src_addr) && !(new_peer && seqno == 0) &&
	((seqno == last_seqno_recv) ||
	 (seqno - last_seqno_recv >= 0x7fffffff))) {
	set_error(c_format("bad sequence number 0x%08x < 0x%08x",
			   XORP_UINT_CAST(seqno),
			   XORP_UINT_CAST(last_seqno_recv)));
	return false;
    }


    MD5_CTX ctx;
    uint8_t digest[MD5_DIGEST_LENGTH];

    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pkt.size() - MD5_DIGEST_LENGTH);
    MD5_Update(&ctx, k->key_data(), k->key_data_bytes());
    MD5_Final(&digest[0], &ctx);

    if (0 != memcmp(&digest[0], &ptr[pkt.size() - MD5_DIGEST_LENGTH],
		    MD5_DIGEST_LENGTH)) {
	set_error(c_format("authentication digest doesn't match local key "
			   "(key ID = %d)", k->id()));
// #define	DUMP_BAD_MD5
#ifdef	DUMP_BAD_MD5
	const char badmd5[] = "/tmp/ospf_badmd5";
	// If the file already exists don't dump anything. The file
	// should contain and only one packet.
	if (-1 == access(badmd5, R_OK)) {
	    XLOG_INFO("Dumping bad MD5 to %s", badmd5);
	    FILE *fp = fopen(badmd5, "w");
	    fwrite(&pkt[0], pkt.size(), 1 , fp);
	    fclose(fp);
	}
#endif
	return false;
    }

    // Update sequence number only after packet has passed digest check
    k->set_last_seqno_recv(src_addr, seqno);

    reset_error();
    return true;
}

bool
MD5AuthHandler::authenticate_outbound(vector<uint8_t>& pkt)
{
    TimeVal now;
    vector<uint8_t> trailer;

    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    _eventloop.current_time(now);

    KeyChain::iterator k = key_at(now.sec());
    if (k == _key_chain.end()) {
	set_error("no valid keys to authenticate outbound data");
	return (false);
    }

    uint8_t *ptr = &pkt[0];

    // Set the authentication type and zero out the checksum.
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET], 0);

    // Set config in the authentication block.
    embed_16(&ptr[Packet::AUTH_PAYLOAD_OFFSET], 0);
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 2] = k->id();
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 3] = MD5_DIGEST_LENGTH;
    embed_32(&ptr[Packet::AUTH_PAYLOAD_OFFSET + 4], k->next_seqno_out());

    // Add the space for the digest at the end of the packet.
    size_t pend = pkt.size();
    pkt.resize(pkt.size() + MD5_DIGEST_LENGTH);
    ptr = &pkt[0];

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pend);
    MD5_Update(&ctx, k->key_data(), k->key_data_bytes());
    MD5_Final(&ptr[pend], &ctx);

    reset_error();
    return (true);
}

void
MD5AuthHandler::reset()
{
    reset_keys();
}

bool
Auth::set_simple_authentication_key(const string& password, string& error_msg)
{
    PlaintextAuthHandler* plaintext_ah = NULL;

    XLOG_ASSERT(_auth_handler != NULL);
    if (_auth_handler->name() != PlaintextAuthHandler::auth_type_name()) {
	set_method(PlaintextAuthHandler::auth_type_name());
    }
    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(_auth_handler);
    XLOG_ASSERT(plaintext_ah != NULL);
    plaintext_ah->set_key(password);

    error_msg.clear();
    return (true);
}

bool
Auth::delete_simple_authentication_key(string& error_msg)
{
    XLOG_ASSERT(_auth_handler != NULL);

    if (_auth_handler->name() != PlaintextAuthHandler::auth_type_name()) {
	//
	// XXX: Here we should return a mismatch error.
	// However, if we are adding both a simple password and MD5 handlers,
	// then the rtrmgr configuration won't match the protocol state.
	// Ideally, the rtrmgr and xorpsh shouldn't allow the user to add
	// both handlers. For the time being we need to live with this
	// limitation and therefore we shouldn't return an error here.
	//
	return (true);
#if 0
	error_msg = c_format("Cannot delete simple password authentication: "
			     "no such is configured");
	return (false);	// XXX: not a simple password authentication handler
#endif
    }

    //
    // Install an empty handler and delete the simple authentication handler
    //
    set_method(NullAuthHandler::auth_type_name());

    error_msg.clear();
    return (true);
}

bool
Auth::set_md5_authentication_key(uint8_t key_id, const string& password,
				 uint32_t start_secs, uint32_t end_secs,
				 string& error_msg)
{
    MD5AuthHandler* md5_ah = NULL;

    XLOG_ASSERT(_auth_handler != NULL);
    if (_auth_handler->name() == MD5AuthHandler::auth_type_name()) {
	md5_ah = dynamic_cast<MD5AuthHandler*>(_auth_handler);
	XLOG_ASSERT(md5_ah != NULL);
	if (md5_ah->add_key(key_id, password, start_secs, end_secs) != true) {
	    error_msg = c_format("MD5 key add failed");
	    return (false);
	}
	return (true);
    }

    // Create a new MD5 authentication handler and delete the old handler
    md5_ah = new MD5AuthHandler(_eventloop);
    if (md5_ah->add_key(key_id, password, start_secs, end_secs) != true) {
	error_msg = c_format("MD5 key add failed");
	delete md5_ah;
	return (false);
    }
    delete _auth_handler;
    _auth_handler = md5_ah;

    return (true);
}

bool
Auth::delete_md5_authentication_key(uint8_t key_id, string& error_msg)
{
    MD5AuthHandler* md5_ah = NULL;

    XLOG_ASSERT(_auth_handler != NULL);

    if (_auth_handler->name() != MD5AuthHandler::auth_type_name()) {
	//
	// XXX: Here we should return a mismatch error.
	// However, if we are adding both a simple password and MD5 handlers,
	// then the rtrmgr configuration won't match the protocol state.
	// Ideally, the rtrmgr and xorpsh shouldn't allow the user to add
	// both handlers. For the time being we need to live with this
	// limitation and therefore we shouldn't return an error here.
	//
	return (true);
#if 0
	error_msg = c_format("Cannot delete MD5 password authentication: "
			     "no such is configured");
	return (false);
#endif
    }

    //
    // Find the MD5 authentication handler to modify
    //
    md5_ah = dynamic_cast<MD5AuthHandler *>(_auth_handler);
    XLOG_ASSERT(md5_ah != NULL);

    //
    // Remove the key
    //
    if (md5_ah->remove_key(key_id) != true) {
	error_msg = c_format("Invalid MD5 key ID %u", XORP_UINT_CAST(key_id));
	return (false);
    }

    //
    // If the last key, then install an empty handler and delete the MD5
    // authentication handler.
    //
    if (md5_ah->key_chain().size() == 0) {
	set_method(NullAuthHandler::auth_type_name());
    }

    return (true);
}
