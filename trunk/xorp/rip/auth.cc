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

#ident "$XORP: xorp/rip/auth.cc,v 1.2 2003/04/23 17:06:48 hodson Exp $"

#include "rip_module.h"

#include "config.h"
#include <functional>
#include <openssl/md5.h>

#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"

#include "constants.hh"
#include "auth.hh"

// ----------------------------------------------------------------------------
// AuthHandlerBase implementation

AuthHandlerBase::~AuthHandlerBase()
{
}

const string&
AuthHandlerBase::error() const
{
    return _err;
}

inline void
AuthHandlerBase::reset_error()
{
    if (_err.empty() == false)
	_err.erase();
}

inline void
AuthHandlerBase::set_error(const string& err)
{
    _err = err;
}

// ----------------------------------------------------------------------------
// NullAuthHandler implementation

uint32_t
NullAuthHandler::head_entries() const
{
    return 0;
}

uint32_t
NullAuthHandler::max_routing_entries() const
{
    return RIPv2_ROUTES_PER_PACKET;
}

const char*
NullAuthHandler::name() const
{
    return "none";
}

bool
NullAuthHandler::authenticate(const uint8_t*		     packet,
			      size_t			     packet_bytes,
			      const PacketRouteEntry<IPv4>*& entries,
			      uint32_t&			     n_entries)
{
    entries = 0;
    n_entries = 0;

    if (packet_bytes > RIPv2_MAX_PACKET_BYTES) {
	set_error(c_format("packet too large (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    } else if (packet_bytes < RIPv2_MIN_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    size_t entry_bytes = packet_bytes - sizeof(RipPacketHeader);
    if (entry_bytes % sizeof(PacketRouteEntry<IPv4>)) {
	set_error(c_format("non-integral route entries (%u bytes)",
			    static_cast<uint32_t>(entry_bytes)));
	return false;
    }

    n_entries = entry_bytes / sizeof(PacketRouteEntry<IPv4>);
    if (n_entries == 0) {
	return true;
    }
    
    entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>
	(packet + sizeof(RipPacketHeader));

    // Reject packet if first entry is authentication data
    if (entries[0].addr_family() == PacketRouteEntry<IPv4>::AUTH_ADDR_FAMILY) {
	set_error(c_format("unexpected authentication data (type %d)",
			   entries[0].tag()));
	entries = 0;
	n_entries = 0;
	return false;
    }

    reset_error();
    return true;
}

uint32_t
NullAuthHandler::authenticate(const uint8_t*	      /* packet */,
			      size_t		      packet_bytes,
			      PacketRouteEntry<IPv4>* /* first_entry */,
			      vector<uint8_t>&	      trailer_data)
{
    trailer_data.resize(0);
    reset_error();
    return (packet_bytes - sizeof(RipPacketHeader)) /
	sizeof(PacketRouteEntry<IPv4>);
}


// ----------------------------------------------------------------------------
// Plaintext handler implementation

uint32_t
PlaintextAuthHandler::head_entries() const
{
    return 1;
}

uint32_t
PlaintextAuthHandler::max_routing_entries() const
{
    return RIPv2_ROUTES_PER_PACKET - 1;
}

const char*
PlaintextAuthHandler::name() const
{
    return "plaintext";
}

void
PlaintextAuthHandler::set_key(const string& plaintext_key)
{
    _key = string(plaintext_key, 0, 16);
}

const string&
PlaintextAuthHandler::key() const
{
    return _key;
}

bool
PlaintextAuthHandler::authenticate(const uint8_t*		  packet,
				   size_t			  packet_bytes,
				   const PacketRouteEntry<IPv4>*& entries,
				   uint32_t&			  n_entries)
{
    entries = 0;
    n_entries = 0;
    
    if (packet_bytes > RIPv2_MAX_PACKET_BYTES) {
	set_error(c_format("packet too large (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    if (packet_bytes < RIPv2_MIN_AUTH_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    size_t entry_bytes = packet_bytes - sizeof(RipPacketHeader);
    if (entry_bytes % sizeof(PacketRouteEntry<IPv4>)) {
	set_error(c_format("non-integral route entries (%u bytes)",
			   static_cast<uint32_t>(entry_bytes)));
	return false;
    }

    const PlaintextPacketRouteEntry4* ppr =
	reinterpret_cast<const PlaintextPacketRouteEntry4*>
	(packet + sizeof(RipPacketHeader));

    if (ppr->addr_family() != PlaintextPacketRouteEntry4::ADDR_FAMILY) {
	set_error("not an authenticated packet");
	return false;
    } else if (ppr->auth_type() != PlaintextPacketRouteEntry4::AUTH_TYPE) {
	set_error("not a plaintext authenticated packet");
	return false;
    }

    string passwd = ppr->password();
    if (passwd != key()) {
	set_error(c_format("wrong password \"%s\"", passwd.c_str()));
	return false;
    }

    reset_error();
    n_entries = entry_bytes / sizeof(PacketRouteEntry<IPv4>) - 1;
    if (n_entries)
	entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>(ppr + 1);
    
    return true;
}

uint32_t
PlaintextAuthHandler::authenticate(const uint8_t*	   packet,
				   size_t		   packet_bytes,
				   PacketRouteEntry<IPv4>* first_entry,
				   vector<uint8_t>&	   trailer_data)
{
    static_assert(sizeof(*first_entry) == 20);
    static_assert(sizeof(PlaintextPacketRouteEntry4) == 20);
    assert(packet + sizeof(RipPacketHeader) ==
	   reinterpret_cast<const uint8_t*>(first_entry));

    PlaintextPacketRouteEntry4* ppr =
	reinterpret_cast<PlaintextPacketRouteEntry4*>(first_entry);
    ppr->initialize(key());

    trailer_data.resize(0);
    reset_error();

    uint32_t n = (packet_bytes - sizeof(RipPacketHeader)) /
	sizeof(PacketRouteEntry<IPv4>) - 1;

    return n;
}


// ----------------------------------------------------------------------------
// MD5AuthHandler::MD5Key implementation

MD5AuthHandler::MD5Key::MD5Key(uint8_t		id,
			      const string&	key,
			      uint32_t		start_secs,
			      XorpTimer		to)
    : _id(id), _start_secs(start_secs), _pkts_recv(false), _lr_sno(0),
      _o_sno(0xfffffffe), _to(to)
{
    string::size_type n = key.copy(_key_data, 16);
    if (n < KEY_BYTES) {
	memset(_key_data + n , 0, KEY_BYTES - n);
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
    if (persistent()) return true;

    // XXX Need to deal with clock wrap.
    if (when_secs - start_secs() > 0x7fffffff) return false;

    // Subtract start time to partially mitigate problems of clock wrap.
    return (when_secs - start_secs() <= end_secs() - start_secs());
}

inline void
MD5AuthHandler::MD5Key::set_last_seqno_recv(uint32_t sno)
{
    _pkts_recv = true;
    _lr_sno = sno;
}


// ----------------------------------------------------------------------------
// MD5AuthHandler implementation

MD5AuthHandler::MD5AuthHandler(EventLoop& e, uint32_t timing_slack_secs)
    : _e(e), _slack_secs(timing_slack_secs)
{
}

uint32_t
MD5AuthHandler::head_entries() const
{
    return 1;
}

uint32_t
MD5AuthHandler::max_routing_entries() const
{
    return RIPv2_ROUTES_PER_PACKET - 1;
}

const char*
MD5AuthHandler::name() const
{
    return "md5";
}

bool
MD5AuthHandler::add_key(uint8_t       id,
			const string& key,
			uint32_t      start_secs,
			uint32_t      end_secs)
{
    if (start_secs > end_secs) {
	return false;
    }

    XorpTimer timeout;
    if (start_secs != end_secs) {
	end_secs += _slack_secs;
	timeout = _e.new_oneoff_at(TimeVal(end_secs, 0),
				   callback(this,
					    &MD5AuthHandler::remove_key, id));
    }

    KeyChain::iterator i = _key_chain.begin();
    while (i != _key_chain.end()) {
	if (id == i->id()) {
	    *i = MD5Key(id, key, start_secs, timeout);
	    return true;
	}
	++i;
    }
    _key_chain.push_back(MD5Key(id, key, start_secs, timeout));
    return true;
}

void
MD5AuthHandler::remove_key(uint8_t id)
{
    KeyChain::iterator i =
	find_if(_key_chain.begin(), _key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), id));
    if (_key_chain.end() != i) {
	_key_chain.erase(i);
    }
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
    _e.current_time(now);
    KeyChain::const_iterator ki = key_at(now.sec());
    if (_key_chain.end() != ki) {
	return ki->id();
    }
    return 0xffff;	// an invalid key id
}

bool
MD5AuthHandler::authenticate(const uint8_t*		    packet,
			     size_t			    packet_bytes,
			     const PacketRouteEntry<IPv4>*& entries,
			     uint32_t&			    n_entries)
{
    static_assert(sizeof(MD5PacketTrailer) == 20);

    entries = 0;
    n_entries = 0;
    
    if (packet_bytes > RIPv2_MAX_PACKET_BYTES + sizeof(MD5PacketTrailer)) {
	set_error(c_format("packet too large (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    if (packet_bytes < RIPv2_MIN_AUTH_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    const MD5PacketRouteEntry4* mpr =
	reinterpret_cast<const MD5PacketRouteEntry4*>(packet +
						      sizeof(RipPacketHeader));

    if (mpr->addr_family() != MD5PacketRouteEntry4::ADDR_FAMILY) {
	set_error("not an authenticated packet");
	return false;
    }

    if (mpr->auth_type() != MD5PacketRouteEntry4::AUTH_TYPE) {
	set_error("not an MD5 authenticated packet");
	return false;
    }

    if (mpr->auth_bytes() != sizeof(MD5PacketTrailer)) {
	set_error(c_format("wrong number of auth bytes (%d != %u)",
			   mpr->auth_bytes(),
			   static_cast<uint32_t>(sizeof(MD5PacketTrailer))));
	return false;
    }

    if (uint32_t(mpr->auth_offset() + mpr->auth_bytes()) != packet_bytes) {
	set_error(c_format("Size of packet does not correspond with "
			   "authentication data offset and size "
			   "(%d + %d != %u).", mpr->auth_offset(),
			   mpr->auth_bytes(),
			   static_cast<uint32_t>(packet_bytes)));
	return false;
    }

    KeyChain::iterator k = find_if(_key_chain.begin(), _key_chain.end(),
				   bind2nd(mem_fun_ref(&MD5Key::id_matches),
					   mpr->key_id()));
    if (_key_chain.end() == k) {
	set_error(c_format("packet with key id %d for which no key is"
			   "configured", mpr->key_id()));
	return false;
    }

    if (k->packets_received() &&
	(mpr->seqno() == k->last_seqno_recv() ||
	 mpr->seqno() - k->last_seqno_recv() >= 0x7fffffff)) {
	set_error(c_format("bad sequence number 0x%08x < 0x%08x",
			   mpr->seqno(), k->last_seqno_recv()));
	return false;
    }

    const MD5PacketTrailer* mpt =
	reinterpret_cast<const MD5PacketTrailer*>(packet +
						  mpr->auth_offset());
    if (mpt->valid() == false) {
	set_error("invalid authentication trailer");
	return false;
    }

    MD5_CTX ctx;
    uint8_t digest[16];

    MD5_Init(&ctx);
    MD5_Update(&ctx, packet, mpr->auth_offset());
    MD5_Update(&ctx, k->key_data(), k->key_data_bytes());
    MD5_Final(digest, &ctx);

    if (memcmp(digest, mpt->data(), mpt->data_bytes()) != 0) {
	set_error(c_format("authentication digest doesn't match local key "
			   "(key id = %d)", k->id()));
	return 0;
    }

    // Update sequence number only after packet has passed digest check
    k->set_last_seqno_recv(mpr->seqno());

    reset_error();
    n_entries = (mpr->auth_offset() - sizeof(RipPacketHeader)) /
	sizeof(PacketRouteEntry<IPv4>) - 1;

    if (n_entries > 0)
	entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>(mpr + 1);

    return true;
}

uint32_t
MD5AuthHandler::authenticate(const uint8_t* 	     packet,
			     size_t	    	     packet_bytes,
			     PacketRouteEntry<IPv4>* first_entry,
			     vector<uint8_t>&	     trailer_data)
{
    MD5PacketRouteEntry4* mpr =
	reinterpret_cast<MD5PacketRouteEntry4*>(first_entry);

    TimeVal now;
    _e.current_time(now);

    KeyChain::iterator k = key_at(now.sec());
    if (k == _key_chain.end()) {
	set_error("no valid keys to authenticate outbound data");
	return 0;
    }

    static_assert(sizeof(MD5PacketTrailer) == 20);
    mpr->initialize(packet_bytes, k->id(), sizeof(MD5PacketTrailer),
		    k->next_seqno_out());

    trailer_data.resize(sizeof(MD5PacketTrailer));
    MD5PacketTrailer* mpt =
	reinterpret_cast<MD5PacketTrailer*>(&trailer_data[0]);
    mpt->initialize();

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, packet, mpr->auth_offset());
    MD5_Update(&ctx, k->key_data(), k->key_data_bytes());
    MD5_Final(mpt->data(), &ctx);

    reset_error();
    return packet_bytes / sizeof(MD5PacketRouteEntry4) - 1;
}
