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

// $XORP: xorp/bgp/parameter.hh,v 1.3 2003/01/29 23:38:12 rizzo Exp $

#ifndef __BGP_PARAMETER_HH__
#define __BGP_PARAMETER_HH__

// address family assignments from
// http:// www.iana.org/assignments/address-family-numbers
// not a complete list
#define AFI_IPV4 1
#define AFI_IPV6 2

// sub-address family assignments from
// RFC 2858
// NLRI = Network Layer Reachability Information
#define SAFI_NLRI_UNICAST 1
#define SAFI_NLRI_MULTICAST 2 
#define SAFI_NLRI_UNICASTMULTICAST 3

#include <sys/types.h>
#include "libxorp/debug.h"
#include <string>
#include "exceptions.hh"

enum ParamType {
    PARAMINVALID = 0,		// param type not set yet.
    PARAMTYPEAUTH = 1,
    PARAMTYPECAP = 2
};

enum CapType {
    CAPABILITYMULTIPROTOCOL = 1,
    CAPABILITYREFRESH = 2,
    CAPABILITYREFRESHOLD = 128, // here for backwards compatibility now 2.
    CAPABILITYMULTIROUTE = 4,
    CAPABILITYUNKNOWN = -1, // used to store unknown cababilities
};

class BGPParameter {
public:
    /**
     * create a new BGPParameter from incoming data.
     * Takes a chunk of memory of size l, returns an object of the
     * appropriate type and actual_length is the number of bytes used
     * from the packet.
     * Throws an exception on error.
     */

    static BGPParameter *create(const uint8_t* d, uint16_t max_len,
                size_t& actual_length) throw(CorruptMessage);

    BGPParameter() : _data(0), _length(0), _type(PARAMINVALID)	{}
    BGPParameter(uint8_t l, const uint8_t* d);
    BGPParameter(const BGPParameter& param);
    virtual ~BGPParameter()			{ delete[] _data; }
    virtual void decode() = 0;
    virtual void encode() const = 0;
    void dump_contents() const ;

    void set_type(ParamType t) {
	debug_msg("_Type set %d\n", t); _type = t;
    }

    ParamType type() const {
	debug_msg("_Type retrieved %d\n", _type); return _type;
    }

    void set_length(int l);

    // XXX check- possible discrepancy between length() and paramlength()
    uint8_t length() const			{
	debug_msg("_length retrieved %d\n", _length-2);
	return _length;
    }

    uint8_t paramlength() const {
	debug_msg("_paramlength retrieved %d\n", _length);
	return _length+2;
    }

    uint8_t* data() const			{
	encode();
	return _data;
    }

    BGPParameter* clone() const;
    virtual string str() const = 0;
protected:
    uint8_t* _data;
    uint8_t _length;
    ParamType _type;
private:
};

/* _Data Parameter here includes the first byte which is the type */
class BGPAuthParameter: public BGPParameter {
public:
    BGPAuthParameter();
    BGPAuthParameter(uint8_t l, const uint8_t* d);
    BGPAuthParameter(uint8_t type, uint8_t length, const uint8_t* data);
    BGPAuthParameter(const BGPAuthParameter& param);
    void encode() const;
    void decode();
    void set_authcode(uint8_t a) {
	debug_msg("_auth_code set %d\n", a);
	_auth_code = a;
	_data[0] = a;
    }

    uint8_t get_authcode() {
	debug_msg("_auth_code retrieved %d\n", _auth_code);
	return _auth_code;
    }

    void set_authdata(const uint8_t* d);
    uint8_t* get_authdata();
    string str() const;
protected:
private:
    uint8_t _auth_code;
    uint8_t* _auth_data;
};


/* BGP Capabilities Negotiation Parameter - 2 */

class BGPCapParameter: public BGPParameter {
public:
    BGPCapParameter();
    BGPCapParameter(uint8_t l, const uint8_t* d);
    BGPCapParameter(const BGPCapParameter& param);
    // ~BGPCapParameter();
    // virtual void decode() = 0;
    // virtual void encode() = 0;
    CapType cap_code() const { return _cap_code; }
    virtual string str() const = 0;
protected:
    CapType _cap_code;
    uint8_t _cap_length;
private:
};

class BGPRefreshCapability : public BGPCapParameter {
public:
    BGPRefreshCapability();
    BGPRefreshCapability(uint8_t l, const uint8_t* d);
    BGPRefreshCapability(const BGPRefreshCapability& cap);
    void decode();
    void encode() const;
    string str() const;
protected:
private:
    bool _old_type_code; /* this should be true if the refresh
			    capability used/will use the obsolete
			    capability code for refresh */
};


class BGPMultiProtocolCapability : public BGPCapParameter {
public:
    BGPMultiProtocolCapability(uint16_t afi, uint8_t safi);
    BGPMultiProtocolCapability(uint8_t l, const uint8_t* d);
    BGPMultiProtocolCapability(const BGPMultiProtocolCapability& cap);
    void decode();
    void encode() const;
    void set_address_family(uint16_t f) { _address_family = f; }
    uint16_t get_address_family() const { return _address_family; }
    void set_subsequent_address_family_id(uint8_t f) { _subsequent_address_family = f; }
    uint8_t get_subsequent_address_family_id() const { return _subsequent_address_family; }
    string str() const;
    bool operator==(const BGPMultiProtocolCapability& him) const;
protected:
private:
    uint16_t _address_family;
    uint8_t _subsequent_address_family;
};

class BGPMultiRouteCapability : public BGPCapParameter {
public:
    BGPMultiRouteCapability();
    BGPMultiRouteCapability(uint8_t l, const uint8_t* d);
    BGPMultiRouteCapability(const BGPMultiRouteCapability& cap);
    void decode();
    void encode() const;
    // void set_address_family(uint16_t f) { _address_family = f; }
    // uint16_t get_address_family() const { return _address_family; }
    // void set_subsequent_address_family_id(uint8_t f) { _subsequent_address_family = f; }
    // uint8_t get_subsequent_address_family_id() const { return _subsequent_address_family; }
    string str() const			{ return "BGP Multiple Route Capability\n"; }
protected:
private:
    // uint16_t _address_family;
    // uint8_t _subsequent_address_family;
};

class BGPUnknownCapability : public BGPCapParameter {
public:
    BGPUnknownCapability();
    BGPUnknownCapability(uint8_t l, const uint8_t* d);
    BGPUnknownCapability(const BGPUnknownCapability& cap);
    void decode();
    void encode() const;
    string str() const			{ return "Unknown BGP Capability\n"; }
    CapType unknown_cap_code() const	{ return _unknown_cap_code; }
protected:
private:
   CapType _unknown_cap_code;
};

#endif // __BGP_PARAMETER_HH__
