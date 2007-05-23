// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/rip/auth.hh,v 1.22 2007/02/16 22:47:13 pavlin Exp $

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
class AuthHandlerBase {
public:
    /**
     * Virtual destructor.
     */
    virtual ~AuthHandlerBase();

    /**
     * Get the effective name of the authentication scheme.
     */
    virtual const char* effective_name() const = 0;

    /**
     * Reset the authentication state.
     */
    virtual void reset() = 0;

    /**
     * Get number of routing entries used by authentication scheme at the
     * head of the RIP packet.
     *
     * @return the number of routing entries used by the authentication scheme
     * at the head of the RIP packet: 0 for unauthenticated packets, 1
     * otherwise.
     */
    virtual uint32_t head_entries() const = 0;

    /**
     * Get maximum number of non-authentication scheme use routing entries
     * in a RIP packet.
     */
    virtual uint32_t max_routing_entries() const = 0;

    /**
     * Inbound authentication method.
     *
     * @param packet pointer to first byte of RIP packet.
     * @param packet_bytes number of bytes in RIP packet.
     * @param entries_ptr output variable set to point to first
     *        entry in packet.  Set to NULL if there are no entries, or
     *        on authentication failure.
     * @param n_entries number of entries in the packet.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     *
     * @return true if packet passes authentication checks, false otherwise.
     */
    virtual bool authenticate_inbound(const uint8_t*	packet,
				      size_t		packet_bytes,
				      const uint8_t*&	entries_ptr,
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
     * Get textual description of last error.
     */
    const string& error() const;

protected:
    /**
     * Reset textual description of last error.
     */
    void reset_error();

    /**
     * Set textual description of latest error.
     */
    void set_error(const string& err);

private:
    string _err;
};


/**
 * @short RIPv2 Authentication handler when no authentication scheme is
 * employed.
 */
class NullAuthHandler : public AuthHandlerBase {
public:
    /**
     * Get the effective name of the authentication scheme.
     */
    const char* effective_name() const;

    /**
     * Get the method-specific name of the authentication scheme.
     *
     * @return the method-specific name of the authentication scheme.
     */
    static const char* auth_type_name();

    /**
     * Reset the authentication state.
     */
    void reset();

    /**
     * Get number of routing entries used by authentication scheme at the
     * head of the RIP packet.
     *
     * @return the number of routing entries used by the authentication scheme
     * at the head of the RIP packet: 0 for unauthenticated packets, 1
     * otherwise.
     */
    uint32_t head_entries() const;

    /**
     * Get maximum number of non-authentication scheme use routing entries
     * in a RIP packet.
     */
    uint32_t max_routing_entries() const;

    /**
     * Inbound authentication method.
     *
     * @param packet pointer to first byte of RIP packet.
     * @param packet_bytes number of bytes in RIP packet.
     * @param entries_ptr output variable set to point to first
     *        entry in packet.  Set to NULL if there are no entries, or
     *        on authentication failure.
     * @param n_entries number of entries in the packet.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     *
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const uint8_t*&			entries_ptr,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

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
    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>&	auth_packets,
			       size_t&			n_routes);
};

/**
 * @short RIPv2 Authentication handler for plaintext scheme.
 */
class PlaintextAuthHandler : public AuthHandlerBase {
public:
    /**
     * Get the effective name of the authentication scheme.
     */
    const char* effective_name() const;

    /**
     * Get the method-specific name of the authentication scheme.
     *
     * @return the method-specific name of the authentication scheme.
     */
    static const char* auth_type_name();

    /**
     * Reset the authentication state.
     */
    void reset();

    /**
     * Get number of routing entries used by authentication scheme at the
     * head of the RIP packet.
     *
     * @return the number of routing entries used by the authentication scheme
     * at the head of the RIP packet: 0 for unauthenticated packets, 1
     * otherwise.
     */
    uint32_t head_entries() const;

    /**
     * Get maximum number of non-authentication scheme use routing entries
     * in a RIP packet.
     */
    uint32_t max_routing_entries() const;

    /**
     * Inbound authentication method.
     *
     * @param packet pointer to first byte of RIP packet.
     * @param packet_bytes number of bytes in RIP packet.
     * @param entries_ptr output variable set to point to first
     *        entry in packet.  Set to NULL if there are no entries, or
     *        on authentication failure.
     * @param n_entries number of entries in the packet.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     *
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const uint8_t*&			entries_ptr,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

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
    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>&	auth_packets,
			       size_t&			n_routes);

    /**
     * Get the authentication key.
     *
     * @return the authentication key.
     */
    const string& key() const;

    /**
     * Set the authentication key.
     *
     * @param plaintext_key the plain-text key.
     */
    void set_key(const string& plaintext_key);

protected:
    string	_key;
};


/**
 * @short RIPv2 Authentication handler for MD5 scheme.
 *
 * Class to check inbound MD5 authenticated packets and add
 * authentication data to outbound RIP packets. The RIP MD5
 * authentication scheme is described in RFC 2082.
 */
class MD5AuthHandler : public AuthHandlerBase {
public:
    /**
     * Class to hold MD5 key information.
     */
    class MD5Key {
    public:
	/**
	 * Construct an MD5 Key.
	 */
	MD5Key(uint8_t		key_id,
	       const string&	key,
	       const TimeVal&	start_timeval,
	       const TimeVal&	end_timeval,
	       XorpTimer	start_timer,
	       XorpTimer	end_timer);

	/**
	 * Get the ID associated with the key.
	 */
	uint8_t		id() const		{ return _id; }

	/**
	 * Get pointer to key data.  The data is of size KEY_BYTES.
	 */
	const char*	key_data() const	{ return _key_data; }

	/**
	 * Get the size of the key data in bytes.
	 */
	uint32_t	key_data_bytes() const	{ return KEY_BYTES; }

	/**
	 * Get key data as a string.
	 */
	string	 	key() const;

	/**
	 * Get the start time of the key.
	 */
	const TimeVal&	start_timeval() const	{ return _start_timeval; }

	/**
	 * Get the end time of the key.
	 */
	const TimeVal&	end_timeval() const	{ return _end_timeval; }

	/**
	 * Get indication of whether key is persistent.
	 */
	bool		is_persistent() const	{ return _is_persistent; }

	/**
	 * Set the flag whether the key is persistent.
	 *
	 * @param v if true the key is persistent.
	 */
	void		set_persistent(bool v)	{ _is_persistent = v; }

	/**
	 * Get whether ID matches a particular value (convenient for STL
	 * algorithms).
	 */
	bool		id_matches(uint8_t o) const	{ return _id == o; }

	/**
	 * Get key validity status of key at a particular time.
	 *
	 * @param when the time to test whether the key is valid.
	 */
	bool	 	valid_at(const TimeVal& when) const;

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
	uint32_t next_seqno_out()		{ return _o_seqno++; }

    protected:
	static const uint32_t KEY_BYTES = 16;

	uint8_t		_id;		// Key ID
	char		_key_data[KEY_BYTES]; // Key data
	TimeVal		_start_timeval;	// Start time of the key
	TimeVal		_end_timeval;	// End time of the key
	bool		_is_persistent;	// True if key is persistent
	map<IPv4, bool>	_pkts_recv;	// True if packets received
	map<IPv4, uint32_t> _lr_seqno;	// Last received seqno
	uint32_t	_o_seqno;	// Next outbound sequence number
	XorpTimer	_start_timer;	// Key start timer
	XorpTimer	_stop_timer;	// Key stop timer

	friend class MD5AuthHandler;
    };

    typedef list<MD5Key> KeyChain;

public:
    /**
     * Constructor
     *
     * @param eventloop the EventLoop instance to used for time reference.
     */
    MD5AuthHandler(EventLoop& eventloop);

    /**
     * Get the effective name of the authentication scheme.
     */
    const char* effective_name() const;

    /**
     * Get the method-specific name of the authentication scheme.
     *
     * @return the method-specific name of the authentication scheme.
     */
    static const char* auth_type_name();

    /**
     * Reset the authentication state.
     */
    void reset();

    /**
     * Get number of routing entries used by authentication scheme at the
     * head of the RIP packet.
     *
     * @return the number of routing entries used by the authentication scheme
     * at the head of the RIP packet: 0 for unauthenticated packets, 1
     * otherwise.
     */
    uint32_t head_entries() const;

    /**
     * Get maximum number of non-authentication scheme use routing entries
     * in a RIP packet.
     */
    uint32_t max_routing_entries() const;

    /**
     * Inbound authentication method.
     *
     * @param packet pointer to first byte of RIP packet.
     * @param packet_bytes number of bytes in RIP packet.
     * @param entries_ptr output variable set to point to first
     *        entry in packet.  Set to NULL if there are no entries, or
     *        on authentication failure.
     * @param n_entries number of entries in the packet.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     *
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const uint8_t*			packet,
			      size_t				packet_bytes,
			      const uint8_t*&			entries_ptr,
			      uint32_t&				n_entries,
			      const IPv4&			src_addr,
			      bool				new_peer);

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
    bool authenticate_outbound(RipPacket<IPv4>&		packet,
			       list<RipPacket<IPv4> *>& auth_packets,
			       size_t&			n_routes);

    /**
     * Add a key to the MD5 key chain.
     *
     * If the key already exists, it is updated with the new settings.
     *
     * @param key_id unique ID associated with key.
     * @param key phrase used for MD5 digest computation.
     * @param start_timeval start time when key becomes valid.
     * @param end_timeval end time when key becomes invalid.
     * @param error_msg the error message (if error).
     * @return true on success, false if end time is less than start time
     * or key has already expired.
     */
    bool add_key(uint8_t	key_id,
		 const string&	key,
		 const TimeVal&	start_timeval,
		 const TimeVal&	end_timeval,
		 string&	error_msg);

    /**
     * Remove a key from the MD5 key chain.
     *
     * @param key_id unique ID of key to be removed.
     * @param error_msg the error message (if error).
     * @return true if the key was found and removed, otherwise false.
     */
    bool remove_key(uint8_t key_id, string& error_msg);

    /**
     * A callback that a key from the MD5 key chain has become valid.
     *
     * @param key_id unique ID of the key that has become valid.
     */
    void key_start_cb(uint8_t key_id);

    /**
     * A callback that a key from the MD5 key chain has expired and is invalid.
     *
     * @param key_id unique ID of the key that has expired.
     */
    void key_stop_cb(uint8_t key_id);

    /**
     * Reset the keys for all sources.
     */
    void reset_keys();

    /**
     * Get all valid keys managed by the MD5AuthHandler.
     *
     * @return list of all valid keys.
     */
    const KeyChain& valid_key_chain() const	{ return _valid_key_chain; }

    /**
     * Get all invalid keys managed by the MD5AuthHandler.
     *
     * @return list of all invalid keys.
     */
    const KeyChain& invalid_key_chain() const	{ return _invalid_key_chain; }

    /**
     * Test where the MD5AuthHandler contains any keys.
     *
     * @return if the MD5AuthHandler contains any keys, otherwise false.
     */
    bool empty() const;

protected:
    EventLoop&	_eventloop;		// The event loop
    KeyChain	_valid_key_chain;	// The set of all valid keys
    KeyChain	_invalid_key_chain;	// The set of all invalid keys
    NullAuthHandler _null_handler;	// Null handler if no valid keys
};

#endif // __RIP_AUTH_HH__
