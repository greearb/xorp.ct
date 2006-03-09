// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rip/auth.hh,v 1.15 2006/02/15 17:52:22 pavlin Exp $

#ifndef __RIP_AUTH_HH__
#define __RIP_AUTH_HH__

#include <list>
#include <map>
#include <vector>
#include "packets.hh"

class EventLoop;

/**
 * @short Base clase for RIPv2 authentication mechanisms.
 *
 * The AuthHandlerBase class defines the interfaces for RIPv2
 * authentication handlers.  Handlers are responsible for
 * authenticating inbound datagrams and adding authentication data to
 * outbound datagrams.
 *
 * Error during authentication set an error buffer that clients may
 * query using the error() method.
 */
class AuthHandlerBase
{
public:
    virtual ~AuthHandlerBase();

    /**
     * Inbound authentication method.
     *
     * @param packet pointer to first byte of RIP packet.
     * @param packet_bytes number of bytes in RIP packet.
     * @param entries_start output variable set to point to first
     *        entry in packet.  Set to 0 if there are no entries, or
     *        on authentication failure.
     * @param n_entries number of entries in the packet.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     *
     * @return true if packet passes authentication checks, false otherwise.
     */
    virtual bool authenticate_inbound(const uint8_t*	packet,
				      size_t		packet_bytes,
				      const PacketRouteEntry<IPv4>*& entries,
				      uint32_t&		n_entries,
				      const IPv4&	src_addr,
				      bool		new_peer) = 0;

    /**
     * Outbound authentication method.
     *
     * Create a list of authenticated packets (one for each valid
     * authentication key). Note that the original packet is also modified
     * and authenticated with the first valid key.
     *
     * @param packet the RIP packet to authenticate.
     * @param auth_packets a return-by-reference list with the
     * authenticated RIP packets (one for each valid authentication key).
     * @param n_routes the return-by-reference number of routes in the packet.
     * @return true if packet was successfully authenticated, false when
     * no valid keys are present.
     */
    virtual bool authenticate_outbound(RipPacket<IPv4>&		packet,
				       list<RipPacket<IPv4> *>& auth_packets,
				       size_t&			n_routes) = 0;

    /**
     * Get number of routing entries used by authentication scheme at the
     * head of the RIP packet.  0 for unauthenticated packets, 1 otherwise.
     */
    virtual uint32_t head_entries() const = 0;

    /**
     * Get maximum number of non-authentication scheme use routing entries
     * in a RIP packet.
     */
    virtual uint32_t max_routing_entries() const = 0;

    /**
     * Get name of authentication scheme.
     */
    virtual const char* name() const = 0;

    /**
     * Get textual description of last error.
     */
    const string& error() const;

protected:
    /**
     * Reset textual description of last error.
     */
    inline void reset_error();

    /**
     * Set textual description of latest error.
     */
    inline void set_error(const string& err);

private:
    string _err;
};


/**
 * @short RIPv2 Authentication handler when no authentication scheme is
 * employed.
 */
class NullAuthHandler : public AuthHandlerBase
{
public:
    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const PacketRouteEntry<IPv4>*&	entries_start,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>&	auth_packets,
			       size_t&			n_routes);

    uint32_t head_entries() const;

    uint32_t max_routing_entries() const;

    const char* name() const;

    static const char* auth_type_name();
};

/**
 * @short RIPv2 Authentication handler for plaintext scheme.
 */
class PlaintextAuthHandler : public AuthHandlerBase
{
public:
    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const PacketRouteEntry<IPv4>*&	entries_start,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>&	auth_packets,
			       size_t&			n_routes);

    uint32_t head_entries() const;

    uint32_t max_routing_entries() const;

    const char* name() const;

    static const char* auth_type_name();

    void set_key(const string& plaintext_key);

    const string& key() const;

protected:
    string _key;
};


/**
 * @short RIPv2 Authentication handler for MD5 scheme.
 *
 * Class to check inbound MD5 authenticated packets and add
 * authentication data to outbound RIP packets. The RIP MD5
 * authentication scheme is described in RFC 2082.
 */
class MD5AuthHandler : public AuthHandlerBase
{
public:
    /**
     * Structure to hold MD5 key information.
     */
    struct MD5Key
    {
	static const uint32_t KEY_BYTES = 16;

	/**
	 * Construct an MD5 Key.
	 */
	MD5Key(uint8_t		key_id,
	       const string&	key,
	       uint32_t		start_secs,
	       XorpTimer	to) ;

	/**
	 * Get the ID associated with the key.
	 */
	inline uint8_t	id() const			{ return _id; }

	/**
	 * Get pointer to key data.  The data is of size KEY_BYTES.
	 */
	const char*	key_data() const		{ return _key_data; }

	/**
	 * Get the size of the key data in bytes.
	 */
	inline uint32_t	key_data_bytes() const		{ return KEY_BYTES; }

	/**
	 * Get key data as a string.
	 */
	string	 	key() const;

	/**
	 * Get the start time of the key.
	 */
	inline uint32_t	start_secs() const		{ return _start_secs; }

	/**
	 * Get the end time of the key.  If the end time is the same as the
	 * start time, the key is persistent and never expires.
	 */
	uint32_t	end_secs() const;

	/**
	 * Get indication of whether key is persistent.
	 */
	bool		persistent() const;

	/**
	 * Get whether ID matches a particular value (convenient for STL
	 * algorithms).
	 */
	inline bool	id_matches(uint8_t o) const	{ return _id == o; }

	/**
	 * Get key validity status of key at a particular time.
	 * @param when_secs time in seconds since midnight 1 Jan 1970.
	 */
	bool	 	valid_at(uint32_t when_secs) const;

	/**
	 * Reset the key for all sources.
	 */
	void		reset();

	/**
	 * Reset the key for a particular source.
	 *
	 * @param src_addr the source address.
	 */
	void		reset(const IPv4& src_addr);

	/**
	 * Indicate whether valid packets have been received from a source
	 * with this key ID.
	 *
	 * @param src_addr the source address.
	 * @return true if a packet has been received from the source,
	 * otherwise false.
	 */
	bool		packets_received(const IPv4& src_addr) const;

	/**
	 * Get last received sequence number from a source.
	 *
	 * @param src_addr the source address.
	 * @return last sequence number seen from the source. Value may be
	 * garbage if no packets have been received (check first with
	 * @ref packets_received()).
	 */
	uint32_t	last_seqno_recv(const IPv4& src_addr) const;

	/**
	 * Set last sequence number received from a source. This method
	 * implicitly set packets received to true.
	 *
	 * @param src_addr the source address.
	 * @param seqno the last sequence number received from the source.
	 */
	void		set_last_seqno_recv(const IPv4& src_addr,
					    uint32_t seqno);

	/**
	 * Get next sequence number for outbound packets.  The counter
	 * is automatically updated with each call of this method.
	 */
	inline uint32_t next_seqno_out()		{ return _o_seqno++; }

    protected:
	uint8_t   _id;
	char	  _key_data[KEY_BYTES];
	uint32_t  _start_secs;
	map<IPv4, bool>	_pkts_recv;	// true if packets received
	map<IPv4, uint32_t> _lr_seqno;	// last received seqno
	uint32_t  _o_seqno;		// next outbound sequence number
	XorpTimer _to;			// expiry timeout

	friend class MD5AuthHandler;
    };

    typedef list<MD5Key> KeyChain;

    /**
     * Get iterator pointing at first key valid at a particular time.
     *
     * @param when_secs time in seconds since midnight 1 Jan 1970.
     */
    KeyChain::const_iterator key_at(uint32_t when_secs) const;

public:
    /**
     * Constructor
     *
     * @param e the EventLoop instance to used for time reference.
     */
    MD5AuthHandler(EventLoop& e);

    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const PacketRouteEntry<IPv4>*&	entries_start,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>& auth_packets,
			       size_t&			n_routes);

    uint32_t head_entries() const;

    uint32_t max_routing_entries() const;

    const char* name() const;

    static const char* auth_type_name();

    /**
     * Add key to MD5 key chain.  If key already exists, it is updated with
     * new settings.  If the start and end times are the same the key
     * is treated as persistant and will not expire.
     *
     * @param key_id unique ID associated with key.
     * @param key phrase used for MD5 digest computation.
     * @param start_secs start time in seconds since midnight 1 Jan 1970.
     * @param end_secs start time in seconds since midnight 1 Jan 1970.
     *
     * @return true on success, false if end time is less than start time
     * or key has already expired.
     */
    bool add_key(uint8_t	key_id,
		 const string&	key,
		 uint32_t	start_secs,
		 uint32_t	end_secs);

    /**
     * Remove key from MD5 key chain.
     *
     * @param key_id unique ID of key to be removed.
     * @return true if the key was found and removed, otherwise false.
     */
    bool remove_key(uint8_t key_id);

    /**
     * A callback to remove key from MD5 key chain.
     *
     * @param key_id unique ID of key to be removed.
     */
    void remove_key_cb(uint8_t key_id) { remove_key(key_id); }

    /**
     * Get currently active key.
     *
     * @return key ID in range 0-255 if key exists,
     * value outside valid range otherwise 256-65535.
     */
    uint16_t currently_active_key() const;

    /**
     * Reset the keys for all sources.
     */
    void reset_keys();

    /**
     * Get all keys managed by MD5AuthHandler.
     *
     * @return list of keys.
     */
    inline const KeyChain& key_chain() const		{ return _key_chain; }

protected:
    /**
     * Get iterator pointing at first key valid at a particular time.
     *
     * @param when_secs time in seconds since midnight 1 Jan 1970.
     */
    KeyChain::iterator key_at(uint32_t when_secs);

protected:
    EventLoop&	_e;
    KeyChain	_key_chain;
};

#endif // __RIP_AUTH_HH__
