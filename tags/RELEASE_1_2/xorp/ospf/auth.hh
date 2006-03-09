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

// $XORP: xorp/ospf/auth.hh,v 1.5 2005/11/13 19:25:34 atanu Exp $

#ifndef __OSPF_AUTH_HH__
#define __OSPF_AUTH_HH__

#include <openssl/md5.h>

#include <map>

class EventLoop;

/**
 * @short Base clase for OSPFv2 authentication mechanisms.
 *
 * The AuthHandlerBase class defines the interfaces for OSPFv2
 * authentication handlers.  Handlers are responsible for
 * authenticating inbound datagrams and adding authentication data to
 * outbound datagrams.
 *
 * Error during authentication set an error buffer that clients may
 * query using the error() method.
 */
class AuthHandlerBase {
 public:
    virtual ~AuthHandlerBase();

    /**
     * Inbound authentication method.
     *
     * @param packet the packet to verify.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     * @return true if packet passes authentication checks, false otherwise.
     */
    virtual bool authenticate_inbound(const vector<uint8_t>&	packet,
				      const IPv4&		src_addr,
				      bool			new_peer) = 0;

    /**
     * Outbound authentication method.
     *
     * @param packet the packet to authenticate.
     * @return true if packet was successfully authenticated, false when
     * no valid keys are present.
     */
    virtual bool authenticate_outbound(vector<uint8_t>& packet) = 0;

    /**
     * Get the name of the authentication scheme.
     *
     * @return the name of the authentication scheme.
     */
    virtual const char* name() const = 0;

    /**
     * Reset the authentication state.
     */
    virtual void reset() = 0;

    /**
     * Additional bytes that will be added to the payload.
     *
     * @return the number of additional bytes that need to be added to
     * the payload.
     */
    virtual uint32_t additional_payload() const = 0;

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
    inline void set_error(const string& error_msg);

private:
    string _error;
};

/**
 * @short OSPFv2 Authentication handler when no authentication scheme is
 * employed.
 */
class NullAuthHandler : public AuthHandlerBase {
public:
    static const OspfTypes::AuType AUTH_TYPE = OspfTypes::NULL_AUTHENTICATION;

    /**
     * Inbound authentication method.
     *
     * @param packet the packet to verify.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const vector<uint8_t>&	packet,
			      const IPv4&		src_addr,
			      bool			new_peer);

    /**
     * Outbound authentication method.
     *
     * @param packet the packet to authenticate.
     * @return true if packet was successfully authenticated, false when
     * no valid keys are present.
     */
    bool authenticate_outbound(vector<uint8_t>& packet);

    /**
     * Get the name of the authentication scheme.
     *
     * @return the name of the authentication scheme.
     */
    const char* name() const;

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
     * Additional bytes that will be added to the payload.
     *
     * @return the number of additional bytes that need to be added to
     * the payload.
     */
    uint32_t additional_payload() const { return 0; }
};

/**
 * @short OSPFv2 Authentication handler for plaintext scheme.
 */
class PlaintextAuthHandler : public AuthHandlerBase {
 public:
    static const OspfTypes::AuType AUTH_TYPE = OspfTypes::SIMPLE_PASSWORD;

    /**
     * Inbound authentication method.
     *
     * @param packet the packet to verify.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const vector<uint8_t>&	packet,
			      const IPv4&		src_addr,
			      bool			new_peer);


    /**
     * Outbound authentication method.
     *
     * @param packet the packet to authenticate.
     * @return true if packet was successfully authenticated, false when
     * no valid keys are present.
     */
    bool authenticate_outbound(vector<uint8_t>& packet);

    /**
     * Get the name of the authentication scheme.
     *
     * @return the name of the authentication scheme.
     */
    const char* name() const;

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
     * Additional bytes that will be added to the payload.
     *
     * @return the number of additional bytes that need to be added to
     * the payload.
     */
    uint32_t additional_payload() const { return 0; }

    /*
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

 private:
    string	_key;
    uint8_t	_key_data[Packet::AUTH_PAYLOAD_SIZE];
};

/**
 * @short OSPFv2 Authentication handler for MD5 scheme.
 *
 * Class to check inbound MD5 authenticated packets and add
 * authentication data to outbound OSPF packets. The OSPFv2 MD5
 * authentication scheme is described in Section D.3 of RFC 2328.
 */
class MD5AuthHandler : public AuthHandlerBase {
public:
    static const OspfTypes::AuType AUTH_TYPE =
	OspfTypes::CRYPTOGRAPHIC_AUTHENTICATION;

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
	       XorpTimer	to);

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
     * @param eventloop the EventLoop instance to used for time reference.
     */
    MD5AuthHandler(EventLoop& eventloop);

    /**
     * Inbound authentication method.
     *
     * @param packet the packet to verify.
     * @param src_addr the source address of the packet.
     * @param new_peer true if this is a new peer.
     * @return true if packet passes authentication checks, false otherwise.
     */
    bool authenticate_inbound(const vector<uint8_t>&	packet,
			      const IPv4&		src_addr,
			      bool			new_peer);

    /**
     * Outbound authentication method.
     *
     * @param packet the packet to authenticate.
     * @return true if packet was successfully authenticated, false when
     * no valid keys are present.
     */
    bool authenticate_outbound(vector<uint8_t>& packet);

    /**
     * Get the name of the authentication scheme.
     *
     * @return the name of the authentication scheme.
     */
    const char* name() const;

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
     * Additional bytes that will be added to the payload.
     *
     * @return the number of additional bytes that need to be added to
     * the payload.
     */
    uint32_t additional_payload() const { return 0; }

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
    EventLoop&	_eventloop;
    KeyChain	_key_chain;
};

/**
 * This is the class that should be instantiated to access
 * authentication.
 */
class Auth {
 public:
    Auth(EventLoop& eventloop) : _eventloop(eventloop), _auth_handler(NULL)
    {
	set_method("none");
    }

    bool set_method(const string& method) {
	if (_auth_handler != NULL) {
	    delete _auth_handler;
	    _auth_handler = NULL;
	}

	if ("none" == method) {
	    _auth_handler = new NullAuthHandler;
	    return true;
	}

	if ("simple" == method) {
	    _auth_handler = new PlaintextAuthHandler;
	    return true;
	}

	if ("md5" == method) {
	    _auth_handler = new MD5AuthHandler(_eventloop);
	    return true;
	}

	// Never allow _auth to be zero.
	set_method("none");

	return false;
    }

    /**
     * Apply the authentication scheme to the packet.
     */
    void generate(vector<uint8_t>& pkt) {
	XLOG_ASSERT(_auth_handler != NULL);
	_auth_handler->authenticate_outbound(pkt);
    }

    /**
     * Verify that this packet has passed the authentication scheme.
     */
    bool verify(vector<uint8_t>& pkt, const IPv4& src_addr, bool new_peer) {
	XLOG_ASSERT(_auth_handler != NULL);
	return _auth_handler->authenticate_inbound(pkt, src_addr, new_peer);
    }

    bool verify(vector<uint8_t>& pkt, const IPv6& src_addr, bool new_peer) {
	UNUSED(pkt);
	UNUSED(src_addr);
	UNUSED(new_peer);
	return true;
    }

    /**
     * Additional bytes that will be added to the payload.
     */
    uint32_t additional_payload() const {
	XLOG_ASSERT(_auth_handler != NULL);
	return _auth_handler->additional_payload();
    }

    const string& error() const {
	XLOG_ASSERT(_auth_handler != NULL);
	return _auth_handler->error();
    }

    /**
     * Called to notify authentication system to reset.
     */
    void reset() {
	XLOG_ASSERT(_auth_handler != NULL);
	_auth_handler->reset();
    }

    /**
     * Set a simple password authentication key.
     *
     * Note that the current authentication handler is replaced with
     * a simple password authentication handler.
     *
     * @param password the password to set.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_simple_authentication_key(const string& password,
				       string& error_msg);

    /**
     * Delete a simple password authentication key.
     *
     * Note that after the deletion the simple password authentication
     * handler is replaced with a Null authentication handler.
     *
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_simple_authentication_key(string& error_msg);

    /**
     * Set an MD5 authentication key.
     *
     * Note that the current authentication handler is replaced with
     * an MD5 authentication handler.
     *
     * @param key_id unique ID associated with key.
     * @param password phrase used for MD5 digest computation.
     * @param start_secs start time in seconds since midnight 1 Jan 1970.
     * @param end_secs start time in seconds since midnight 1 Jan 1970.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_md5_authentication_key(uint8_t key_id, const string& password,
				    uint32_t start_secs, uint32_t end_secs,
				    string& error_msg);

    /**
     * Delete an MD5 authentication key.
     *
     * Note that after the deletion if there are no more valid MD5 keys,
     * the MD5 authentication handler is replaced with a Null authentication
     * handler.
     *
     * @param key_id the ID of the key to delete.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_md5_authentication_key(uint8_t key_id, string& error_msg);

 private:
    EventLoop&		_eventloop;
    AuthHandlerBase*	_auth_handler;
};

#endif // __OSPF_AUTH_HH__
