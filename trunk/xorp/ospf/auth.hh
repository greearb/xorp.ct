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

// $XORP: xorp/ospf/auth.hh,v 1.3 2005/11/13 05:34:20 atanu Exp $

#ifndef __OSPF_AUTH_HH__
#define __OSPF_AUTH_HH__

#include <openssl/md5.h>

class AuthBase {
 public:

    virtual ~AuthBase() {}

    void set_password(string& password) {
	_password = password;
    }

    string get_password() const {
	return _password;
    }

    /**
     * Apply the authentication scheme to the packet.
     */
    virtual void generate(vector<uint8_t>& /*pkt*/) {}

    /**
     * Verify that this packet has passed the authentication scheme.
     */
    virtual bool verify(vector<uint8_t>& /*pkt*/) {
	return true;
    }

    /**
     * Additional bytes that will be added to the payload.
     */
    virtual uint32_t additional_payload() const {
	return 0;
    }

    void set_verify_error(string& error) {
	_verify_error = error;
    }

    string get_verify_error() {
	return _verify_error;
    }

    /**
     * Called to notify authentication system to reset.
     */
    virtual void reset() {}

private:
    string _password;
    string _verify_error;
};

class AuthNone : public AuthBase {
    
};

class AuthPlainText : public AuthBase {
 public:
    static const OspfTypes::AuType AUTH_TYPE = OspfTypes::SIMPLE_PASSWORD;

    void generate(vector<uint8_t>& pkt);
    bool verify(vector<uint8_t>& pkt);
    void reset();

 private:
    uint8_t _auth[Packet::AUTH_PAYLOAD_SIZE];
};

class AuthMD5 : public AuthBase {
    static const OspfTypes::AuType AUTH_TYPE =
	OspfTypes::CRYPTOGRAPHIC_AUTHENTICATION;
    static const uint16_t KEY_ID = 1;

    void generate(vector<uint8_t>& pkt);
    bool verify(vector<uint8_t>& pkt);
    void reset();

 private:
    uint32_t _inbound_seqno;
    uint32_t _outbound_seqno;
    uint8_t _key[MD5_DIGEST_LENGTH];
};

class Auth {
 public:
    Auth() : _auth(0)
    {
	set_method("none");
    }

    bool set_method(string method) {
	delete _auth;
	_auth = 0;

	if ("none" == method) {
	    _auth = new AuthNone;
	    return true;;
	}

	if ("plaintext" == method) {
	    _auth = new AuthPlainText;
	    return true;;
	}

	if ("md5" == method) {
	    _auth = new AuthMD5;
	    return true;;
	}

	// Never allow _auth to be zero.
	set_method("none");
	
	return false;
    }

    void set_password(string& password) {
	XLOG_ASSERT(_auth);
	_auth->set_password(password);
    }

    /**
     * Apply the authentication scheme to the packet.
     */
    void generate(vector<uint8_t>& pkt) {
	XLOG_ASSERT(_auth);
	_auth->generate(pkt);
    }

    /**
     * Verify that this packet has passed the authentication scheme.
     */
    bool verify(vector<uint8_t>& pkt) {
	XLOG_ASSERT(_auth);
	return _auth->verify(pkt);
    }

    /**
     * Additional bytes that will be added to the payload.
     */
    uint32_t additional_payload() const {
	XLOG_ASSERT(_auth);
	return _auth->additional_payload();
    }

    string get_verify_error() {
	XLOG_ASSERT(_auth);
	return _auth->get_verify_error();
    }

    /**
     * Called to notify authentication system to reset.
     */
    void reset() {
	XLOG_ASSERT(_auth);
	_auth->reset();
    }
    
 private:
    AuthBase *_auth;
};

#endif // __OSPF_AUTH_HH__
