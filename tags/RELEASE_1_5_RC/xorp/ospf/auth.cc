// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/auth.cc,v 1.17 2007/02/16 22:46:40 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

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
#include "libproto/packet.hh"

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
NullAuthHandler::effective_name() const
{
    return auth_type_name();
}

const char*
NullAuthHandler::auth_type_name()
{
    return "none";
}

void
NullAuthHandler::reset()
{
}

uint32_t
NullAuthHandler::additional_payload() const
{
    return 0;
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

// ----------------------------------------------------------------------------
// Plaintext handler implementation

const char*
PlaintextAuthHandler::effective_name() const
{
    return auth_type_name();
}

const char*
PlaintextAuthHandler::auth_type_name()
{
    return "simple";
}

void
PlaintextAuthHandler::reset()
{
}

uint32_t
PlaintextAuthHandler::additional_payload() const
{
    return 0;
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

// ----------------------------------------------------------------------------
// MD5AuthHandler::MD5Key implementation

MD5AuthHandler::MD5Key::MD5Key(uint8_t		key_id,
			       const string&	key,
			       const TimeVal&	start_timeval,
			       const TimeVal&	end_timeval,
			       const TimeVal&	max_time_drift,
			       XorpTimer	start_timer,
			       XorpTimer	stop_timer)
    : _id(key_id),
      _start_timeval(start_timeval),
      _end_timeval(end_timeval),
      _max_time_drift(max_time_drift),
      _is_persistent(false),
      _o_seqno(0),
      _start_timer(start_timer),
      _stop_timer(stop_timer)
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

bool
MD5AuthHandler::MD5Key::valid_at(const TimeVal& when) const
{
    if (is_persistent())
	return true;

    return ((_start_timeval <= when) && (when <= _end_timeval));
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
MD5AuthHandler::effective_name() const
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	return (_null_handler.effective_name());
    }

    return auth_type_name();
}

const char*
MD5AuthHandler::auth_type_name()
{
    return "md5";
}

void
MD5AuthHandler::reset()
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	_null_handler.reset();
	return;
    }

    reset_keys();
}

uint32_t
MD5AuthHandler::additional_payload() const
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	return (_null_handler.additional_payload());
    }

    return 0;
}

bool
MD5AuthHandler::authenticate_inbound(const vector<uint8_t>& pkt,
				     const IPv4& src_addr, bool new_peer)
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	if (_null_handler.authenticate_inbound(pkt, src_addr, new_peer)
	    != true) {
	    set_error(_null_handler.error());
	    return (false);
	}
	reset_error();
	return (true);
    }

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

    KeyChain::iterator k = find_if(_valid_key_chain.begin(),
				   _valid_key_chain.end(),
				   bind2nd(mem_fun_ref(&MD5Key::id_matches),
					   key_id));
    if (k == _valid_key_chain.end()) {
	set_error(c_format("packet with key ID %d for which no key is "
			   "configured", key_id));
	return false;
    }
    MD5Key* key = &(*k);

    if (new_peer)
	key->reset(src_addr);

    uint32_t last_seqno_recv = key->last_seqno_recv(src_addr);
    if (key->packets_received(src_addr) && !(new_peer && seqno == 0) &&
	(seqno - last_seqno_recv >= 0x7fffffff)) {
	set_error(c_format("bad sequence number 0x%08x < 0x%08x",
			   XORP_UINT_CAST(seqno),
			   XORP_UINT_CAST(last_seqno_recv)));
	return false;
    }


    MD5_CTX ctx;
    uint8_t digest[MD5_DIGEST_LENGTH];

    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pkt.size() - MD5_DIGEST_LENGTH);
    MD5_Update(&ctx, key->key_data(), key->key_data_bytes());
    MD5_Final(&digest[0], &ctx);

    if (0 != memcmp(&digest[0], &ptr[pkt.size() - MD5_DIGEST_LENGTH],
		    MD5_DIGEST_LENGTH)) {
	set_error(c_format("authentication digest doesn't match local key "
			   "(key ID = %d)", key->id()));
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
    key->set_last_seqno_recv(src_addr, seqno);

    reset_error();
    return true;
}

bool
MD5AuthHandler::authenticate_outbound(vector<uint8_t>& pkt)
{
    TimeVal now;
    vector<uint8_t> trailer;

    _eventloop.current_time(now);

    MD5Key* key = best_outbound_key(now);

    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (key == NULL) {
	if (_null_handler.authenticate_outbound(pkt) != true) {
	    set_error(_null_handler.error());
	    return (false);
	}
	reset_error();
	return (true);
    }

    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];

    // Set the authentication type and zero out the checksum.
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET], 0);

    // Set config in the authentication block.
    embed_16(&ptr[Packet::AUTH_PAYLOAD_OFFSET], 0);
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 2] = key->id();
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 3] = MD5_DIGEST_LENGTH;
    embed_32(&ptr[Packet::AUTH_PAYLOAD_OFFSET + 4], key->next_seqno_out());

    // Add the space for the digest at the end of the packet.
    size_t pend = pkt.size();
    pkt.resize(pkt.size() + MD5_DIGEST_LENGTH);
    ptr = &pkt[0];

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pend);
    MD5_Update(&ctx, key->key_data(), key->key_data_bytes());
    MD5_Final(&ptr[pend], &ctx);

    reset_error();
    return (true);
}

bool
MD5AuthHandler::add_key(uint8_t		key_id,
			const string&	key,
			const TimeVal&	start_timeval,
			const TimeVal&	end_timeval,
			const TimeVal&	max_time_drift,
			string&		error_msg)
{
    TimeVal now, adj_timeval;
    XorpTimer start_timer, end_timer;
    string dummy_error_msg;

    _eventloop.current_time(now);

    if (start_timeval > end_timeval) {
	error_msg = c_format("Start time is later than the end time");
	return false;
    }
    if (end_timeval < now) {
	error_msg = c_format("End time is in the past");
	return false;
    }

    // Adjust the time when we start using the key to accept messages
    adj_timeval = start_timeval;
    if (adj_timeval >= max_time_drift)
	adj_timeval -= max_time_drift;
    else
	adj_timeval = TimeVal::ZERO();
    if (adj_timeval > now) {
	start_timer = _eventloop.new_oneoff_at(
	    adj_timeval,
	    callback(this, &MD5AuthHandler::key_start_cb, key_id));
    }

    // Adjust the time when we stop using the key to accept messages
    adj_timeval = end_timeval;
    if (TimeVal::MAXIMUM() - max_time_drift > adj_timeval)
	adj_timeval += max_time_drift;
    else
	adj_timeval = TimeVal::MAXIMUM();
    if (adj_timeval != TimeVal::MAXIMUM()) {
	end_timer = _eventloop.new_oneoff_at(
	    adj_timeval,
	    callback(this, &MD5AuthHandler::key_stop_cb, key_id));
    }

    //
    // XXX: If we are using the last authentication key that has expired,
    // move it to the list of invalid keys.
    //
    if (_valid_key_chain.size() == 1) {
	MD5Key& key = _valid_key_chain.front();
	if (key.is_persistent()) {
	    key.set_persistent(false);
	    _invalid_key_chain.push_back(key);
	    _valid_key_chain.pop_front();
	}
    }

    // XXX: for simplicity just try to remove the key even if it doesn't exist
    remove_key(key_id, dummy_error_msg);

    // Add the new key to the appropriate chain
    MD5Key new_key = MD5Key(key_id, key, start_timeval, end_timeval,
			    max_time_drift, start_timer, end_timer);
    if (start_timer.scheduled())
	_invalid_key_chain.push_back(new_key);
    else
	_valid_key_chain.push_back(new_key);

    return true;
}

bool
MD5AuthHandler::remove_key(uint8_t key_id, string& error_msg)
{
    KeyChain::iterator i;

    // Check among all valid keys
    i = find_if(_valid_key_chain.begin(), _valid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _valid_key_chain.end()) {
	_valid_key_chain.erase(i);
	return true;
    }

    // Check among all invalid keys
    i = find_if(_invalid_key_chain.begin(), _invalid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _invalid_key_chain.end()) {
	_invalid_key_chain.erase(i);
	return true;
    }

    error_msg = c_format("No such key");
    return false;
}

void
MD5AuthHandler::key_start_cb(uint8_t key_id)
{
    KeyChain::iterator i;

    // Find the key among all invalid keys and move it to the valid keys
    i = find_if(_invalid_key_chain.begin(), _invalid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _invalid_key_chain.end()) {
	MD5Key& key = *i;
	_valid_key_chain.push_back(key);
	_invalid_key_chain.erase(i);
    }
}

void
MD5AuthHandler::key_stop_cb(uint8_t key_id)
{
    KeyChain::iterator i;

    // Find the key among all valid keys and move it to the invalid keys
    i = find_if(_valid_key_chain.begin(), _valid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _valid_key_chain.end()) {
	MD5Key& key = *i;
	//
	// XXX: If the last key expires then keep using it as per
	// RFC 2328 Appendix D.3 until the lifetime is extended, the key
	// is deleted by network management, or a new key is configured.
	//
	if (_valid_key_chain.size() == 1) {
	    XLOG_WARNING("Last authentication key (key ID = %u) has expired. "
			 "Will keep using it until its lifetime is extended, "
			 "the key is deleted, or a new key is configured.",
			 key_id);
	    key.set_persistent(true);
	    return;
	}
	_invalid_key_chain.push_back(key);
	_valid_key_chain.erase(i);
    }
}

/**
 * Select the best key for outbound messages.
 *
 * The chosen key is the one with most recent start-time in the past.
 * If there is more than one key that matches the criteria, then select
 * the key with greatest ID.
 *
 * @param now current time.
 */
MD5AuthHandler::MD5Key*
MD5AuthHandler::best_outbound_key(const TimeVal& now)
{
    MD5Key* best_key = NULL;
    KeyChain::iterator iter;

    for (iter = _valid_key_chain.begin();
	 iter != _valid_key_chain.end();
	 ++iter) {
	MD5Key* key = &(*iter);

	if (! key->valid_at(now))
	    continue;

	if (best_key == NULL) {
	    best_key = key;
	    continue;
	}

	// Select the key with most recent start-time
	if (best_key->start_timeval() > key->start_timeval())
	    continue;
	if (best_key->start_timeval() < key->start_timeval()) {
	    best_key = key;
	    continue;
	}
	
	// The start time is same hence select the key with greatest ID
	if (best_key->id() > key->id())
	    continue;
	if (best_key->id() < key->id()) {
	    best_key = key;
	    continue;
	}
	XLOG_UNREACHABLE();	// XXX: we cannot have two keys with same ID
    }

    return (best_key);
}

void
MD5AuthHandler::reset_keys()
{
    KeyChain::iterator iter;

    for (iter = _valid_key_chain.begin();
	 iter != _valid_key_chain.end();
	 ++iter) {
	iter->reset();
    }
}

bool
MD5AuthHandler::empty() const
{
    return (_valid_key_chain.empty() && _invalid_key_chain.empty());
}

// ----------------------------------------------------------------------------
// Auth implementation

bool
Auth::set_simple_authentication_key(const string& password, string& error_msg)
{
    PlaintextAuthHandler* plaintext_ah = NULL;

    XLOG_ASSERT(_auth_handler != NULL);

    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(_auth_handler);
    if (plaintext_ah == NULL) {
	set_method(PlaintextAuthHandler::auth_type_name());
    }
    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(_auth_handler);
    XLOG_ASSERT(plaintext_ah != NULL);
    plaintext_ah->set_key(password);

    error_msg = "";
    return (true);
}

bool
Auth::delete_simple_authentication_key(string& error_msg)
{
    PlaintextAuthHandler* plaintext_ah = NULL;
    
    XLOG_ASSERT(_auth_handler != NULL);

    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(_auth_handler);
    if (plaintext_ah != NULL) {
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

    error_msg = "";
    return (true);
}

bool
Auth::set_md5_authentication_key(uint8_t key_id, const string& password,
				 const TimeVal& start_timeval,
				 const TimeVal& end_timeval,
				 const TimeVal& max_time_drift,
				 string& error_msg)
{
    MD5AuthHandler* md5_ah = NULL;

    XLOG_ASSERT(_auth_handler != NULL);

    md5_ah = dynamic_cast<MD5AuthHandler*>(_auth_handler);
    if (md5_ah != NULL) {
	if (md5_ah->add_key(key_id, password, start_timeval, end_timeval,
			    max_time_drift, error_msg) != true) {
	    error_msg = c_format("MD5 key add failed: %s", error_msg.c_str());
	    return (false);
	}
	return (true);
    }

    // Create a new MD5 authentication handler and delete the old handler
    md5_ah = new MD5AuthHandler(_eventloop);
    if (md5_ah->add_key(key_id, password, start_timeval, end_timeval,
			max_time_drift, error_msg) != true) {
	error_msg = c_format("MD5 key add failed: %s", error_msg.c_str());
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

    md5_ah = dynamic_cast<MD5AuthHandler*>(_auth_handler);
    if (md5_ah != NULL) {
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

    XLOG_ASSERT(md5_ah != NULL);

    //
    // Remove the key
    //
    if (md5_ah->remove_key(key_id, error_msg) != true) {
	error_msg = c_format("Invalid MD5 key ID %u: %s",
			     XORP_UINT_CAST(key_id), error_msg.c_str());
	return (false);
    }

    //
    // If the last key, then install an empty handler and delete the MD5
    // authentication handler.
    //
    if (md5_ah->empty()) {
	set_method(NullAuthHandler::auth_type_name());
    }

    return (true);
}
