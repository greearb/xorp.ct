// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/parameter.cc,v 1.5 2003/01/30 01:10:17 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"

#include "packet.hh"

extern void dump_bytes(const uint8_t *d, size_t l);

/* **************** BGPParameter *********************** */

BGPParameter::BGPParameter(uint8_t l, const uint8_t* d)
{
    // Allocate the Data buffer
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l; // length is the buffer length including parameter header
}

BGPParameter::BGPParameter(const BGPParameter& param)
{
    _type = param._type;
    if (_data != NULL) {
	_length = param._length;
	uint8_t *data = new uint8_t[_length];
	memcpy(data, param._data, _length);
	_data = data;
    } else {
	_data = NULL;
	_length = 0;
    }
}

void
BGPParameter::set_length(int l)
{
    if (_data != NULL)
	delete[] _data;

    assert (l >= 2 && l < 256);
    debug_msg("_length set %d\n", l);
    _length = l;
    _data = new uint8_t[_length];
    _data[1] = _length-2;
}

BGPParameter*
BGPParameter::clone() const
{
    BGPParameter *clone = NULL;
    switch (_type) {
    case PARAMTYPEAUTH:
	clone = new BGPAuthParameter((const BGPAuthParameter&)
				     (*this));
	break;
    case PARAMTYPECAP: {
	const BGPCapParameter* cparam = (const BGPCapParameter*)this;
	switch (cparam->cap_code()) {
	case CAPABILITYMULTIPROTOCOL:
	    clone = new BGPMultiProtocolCapability\
		((const BGPMultiProtocolCapability&)(*this));
	    break;
	case CAPABILITYREFRESHOLD:
	    abort();
	    break;
	case CAPABILITYREFRESH:
	    clone = new BGPRefreshCapability\
		((const BGPRefreshCapability&)(*this));
	    break;
	case CAPABILITYMULTIROUTE:
	    clone = new BGPMultiRouteCapability\
		((const BGPMultiRouteCapability&)(*this));
	    break;
	default:
	    abort();
	}
	break;
    }
    default:
	abort();
    }
    return clone;
}


/* **************** BGPAuthParameter *********************** */

BGPAuthParameter::BGPAuthParameter()
{
    _type = PARAMTYPEAUTH;
    _length = 255; /* no idea really*/
    _data = new uint8_t[_length];
    abort();
}

BGPAuthParameter::BGPAuthParameter(uint8_t l, const uint8_t* d)
    : BGPParameter(l, d)
{
    debug_msg("BGPAuthParameter() constructor called\n");
    _type = PARAMTYPEAUTH;
    decode();
    debug_msg("_type %d _length %d (total length %d)\n",
	      _type, _length, _length+2);
}

BGPAuthParameter::BGPAuthParameter(const BGPAuthParameter& param)
    : BGPParameter(param)
{
    _auth_code = param._auth_code;

    if (param._data != NULL) {
        _length = param._length;
        uint8_t *p = new uint8_t[_length];
        memcpy(p, param._data, _length);
        _data = p;
    } else {
        _length = 0;
        _data = NULL;
    }
}

void
BGPAuthParameter::set_authdata(const uint8_t* d)
{
    memcpy(_data+2, d, _length-2);
}

uint8_t*
BGPAuthParameter::get_authdata()
{
    return (_data + 2);
}

void
BGPAuthParameter::decode()
{
    debug_msg("decode in BGPAuthParameter called\n");
    _type = PARAMTYPEAUTH;
    // Plus any authentication related decoding.
    // XXX TBD
    abort();
}

void
BGPAuthParameter::encode() const
{
    // stuff
}


string
BGPAuthParameter::str() const
{
    string s;
    s = "BGP Authenitication Parameter\n";
    return s;
}
/* **************** BGPCapParameter *********************** */

BGPCapParameter::BGPCapParameter()
{
    _type = PARAMTYPECAP;
    _cap_length = 0;
}

BGPCapParameter::BGPCapParameter(uint8_t l, const uint8_t* d)
    : BGPParameter(l, d)
{
    _type = PARAMTYPECAP;
}

BGPCapParameter::BGPCapParameter(const BGPCapParameter& param)
    : BGPParameter(param)
{
    _cap_code = param._cap_code;
    _cap_length = param._cap_length;
}

/* ************** BGPRefreshCapability - ****************** */

BGPRefreshCapability::BGPRefreshCapability()
{
    debug_msg("BGPRefreshCapability() constructor\n");
    _cap_code = CAPABILITYREFRESH;
    _length = 4;
    _data = new uint8_t[_length];
    _old_type_code = false;
}

BGPRefreshCapability::BGPRefreshCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGPRefreshCapability(uint8_t, uint8_t*) constructor called\n");
    debug_msg("_type %d _length %d (total length %d) \n",
	      _type, _length, _length+2);
    decode();
}

BGPRefreshCapability::
BGPRefreshCapability(const BGPRefreshCapability& param)
    : BGPCapParameter(param)
{
    _old_type_code = param._old_type_code;

    if (param._data != NULL) {
        _length = param._length;
        uint8_t *p = new uint8_t[_length];
        memcpy(p, param._data, _length);
        _data = p;
    } else {
        _length = 0;
        _data = NULL;
    }
}

void
BGPRefreshCapability::decode()
{
    _type = (ParamType)*(_data);
    assert(_type == PARAMTYPECAP);

    _length = (uint8_t)*(_data+1) + 2; // includes 2 byte header
    assert(_length == 4);

    _cap_code = (CapType)*(_data+2);
    if (_cap_code == CAPABILITYREFRESHOLD) {
	_old_type_code = true;
	_cap_code = CAPABILITYREFRESH;
    } else {
	_old_type_code = false;
    }
    assert(_cap_code == CAPABILITYREFRESH);

    _cap_length = (uint8_t)*(_data+3);
    if (_cap_length > 0) {
	debug_msg("Throw exception\n");
	xorp_throw(CorruptMessage, c_format("Refresh Capability length %d is greater than zero.", _cap_length), OPENMSGERROR, UNSPECIFIED);
	abort();
    }

}

void
BGPRefreshCapability::encode() const
{
    debug_msg("Encoding a BGP refresh capability parameter\n");

    _data[0] = PARAMTYPECAP;
    _data[1] = 2;
    if (_old_type_code)
	_data[2] = CAPABILITYREFRESHOLD;
    else
	_data[2] = CAPABILITYREFRESH;
    _data[3] = 0;

    debug_msg("length set as %d\n", _length);
}

string
BGPRefreshCapability::str() const
{
    string s;
    s = "BGP Refresh Capability\n";
    return s;
}



/* ************** BGPMultiProtocolCapability - ****************** */

BGPMultiProtocolCapability::
BGPMultiProtocolCapability( uint16_t afi, uint8_t safi )
{
    _cap_code = CAPABILITYMULTIPROTOCOL;
    _length = 8;
    _data = new uint8_t[_length];
    _address_family = afi;
    if (safi > 255)
	abort();
    _subsequent_address_family = safi;
}

BGPMultiProtocolCapability::
BGPMultiProtocolCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGPMultiProtocolCapability(uint8_t, uint8_t*) constructor called\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n", _type, _length, _length+2);
}

BGPMultiProtocolCapability::
BGPMultiProtocolCapability(const BGPMultiProtocolCapability& param)
    : BGPCapParameter(param)
{
    _address_family = param._address_family;
    _subsequent_address_family =
	param._subsequent_address_family;

    if (param._data != NULL) {
        _length = param._length;
        uint8_t *p = new uint8_t[_length];
        memcpy(p, param._data, _length);
        _data = p;
    } else {
        _length = 0;
        _data = NULL;
    }
}

void
BGPMultiProtocolCapability::decode()
{
    _type = (ParamType)*(_data);
    assert(_type == PARAMTYPECAP);

    _length = (uint8_t)*(_data+1) + 2; // includes 2 byte header
    _cap_code = (CapType)*(_data+2);
    assert(_cap_code == CAPABILITYMULTIPROTOCOL);

    _cap_length = (uint8_t)*(_data+3);
    _address_family = ntohs((uint16_t)*(_data+4));
    debug_msg("address family %d\n", _address_family);
    _subsequent_address_family = (uint8_t)*(_data+7);
}

void
BGPMultiProtocolCapability::encode() const
{
    debug_msg("Encoding a BGP multi protocol capability parameter\n");
    _data[0] = PARAMTYPECAP;
    _data[1] = 6; // parameter length
    _data[2] = CAPABILITYMULTIPROTOCOL;
    _data[3] = 4; // capability length
    uint16_t k = htons((uint16_t)_address_family) ; // address family
    memcpy(&_data[4],&k, 2);
    _data[6] = 0 ; // Reserved
    _data[7] = _subsequent_address_family ; // SAFI
}

string
BGPMultiProtocolCapability::str() const
{
    string s;
    s = "BGP Multiple Protocol Capability\n";
    return s;
}

bool
BGPMultiProtocolCapability::operator==(const BGPMultiProtocolCapability& him)
    const
{
    if (_address_family != him.get_address_family())
	return false;
    if (_subsequent_address_family != him.get_subsequent_address_family_id())
	return false;
    return true;
}

/* ************** BGPMultiRouteCapability - ****************** */
// Unsure of the exact packet format of this capability
BGPMultiRouteCapability::
BGPMultiRouteCapability()
{
    _cap_code = CAPABILITYMULTIROUTE;
    _length = 8;
    _data = new uint8_t[_length];
}

BGPMultiRouteCapability::
BGPMultiRouteCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGPMultiRouteCapability(uint8_t, uint8_t*) constructor called\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n", _type, _length, _length+2);
}

BGPMultiRouteCapability::
BGPMultiRouteCapability(const BGPMultiRouteCapability& param)
    : BGPCapParameter(param)
{
    // _address_family = param._address_family;
    // _subsequent_address_family = param._subsequent_address_family;

    if (param._data != NULL) {
        _length = param._length;
        uint8_t *p = new uint8_t[_length];
        memcpy(p, param._data, _length);
        _data = p;
    } else {
        _length = 0;
        _data = NULL;
    }
}

void
BGPMultiRouteCapability::decode()
{
    _type = (ParamType)*(_data);
    assert(_type == PARAMTYPECAP);

    _length = (uint8_t)*(_data+1) + 2; // includes 2 byte header

    _cap_code = (CapType)*(_data+2);
    assert(_cap_code == CAPABILITYMULTIROUTE);

    _cap_length = (uint8_t)*(_data+3);
    // _address_family = ntohs((uint16_t &)*(_data+4));
    // _subsequent_address_family = (uint8_t &)*(_data+7);
}

void
BGPMultiRouteCapability::encode() const
{
    debug_msg("Encoding a BGP multi route to host capability parameter\n");
    _data[0] = PARAMTYPECAP;
    _data[1] = 6;
    _data[2] = CAPABILITYMULTIROUTE;
    // _data[3] = 4;
    // _data[4] = htons(_address_family) ; // address family
    // _data[6] = 0 ; // Reserved
    // _data[7] = _subsequent_address_family ; // SAFI
}

/************** BGPUnknownCapability - ****************** */
// This is used when we don't understand / support a capability
// we are sent.
BGPUnknownCapability::
BGPUnknownCapability()
{
    // we shouldn't ever create one of these.
    // abort() ?
    _cap_code = CAPABILITYUNKNOWN;
    _length = 0;
    _data = NULL;
}

BGPUnknownCapability::
BGPUnknownCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGPUnkownCapability(uint8_t, uint8_t*) constructor called\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n", _type, _length, _length+2);
}

BGPUnknownCapability::
BGPUnknownCapability(const BGPUnknownCapability& param)
    : BGPCapParameter(param)
{
    if (param._data != NULL) {
        _length = param._length;
        uint8_t *p = new uint8_t[_length];
        memcpy(p, param._data, _length);
        _data = p;
    } else {
        _length = 0;
        _data = NULL;
    }
}

void
BGPUnknownCapability::decode()
{
    debug_msg("decoding unknown capability\n");
    _type = (ParamType)*(_data);
    assert(_type == PARAMTYPECAP);

    _length = (uint8_t)*(_data+1) + 2; // includes 2 byte header

    _unknown_cap_code = (CapType)*(_data+2); // the original capability code
    _cap_code = CAPABILITYUNKNOWN; // hard code capability to be UNKNOWN.

    // don't check the capability type, since we already know we
    // don't know what it is.
    // assert(_cap_code == CAPABILITYUNKNOWN);

    _cap_length = (uint8_t)*(_data+3);
    // _data pointer holds pointer to the rest of the information
    // can't decode it further since we don't know what it means.
}

void
BGPUnknownCapability::encode() const
{
    /*
    debug_msg("Encoding an Unknown capability parameter\n");
    _data[0] = PARAMTYPECAP;
    _data[1] = 8;
    _data[2] = CAPABILITYUNKNOWN;
    */
    // should never do this
    debug_msg("we shouldn't ever send an unknown capability\n");
    // abort();
}

BGPParameter *
BGPParameter::create(const uint8_t* d, uint16_t max_len, size_t& len)
	throw(CorruptMessage)
{
    debug_msg("max_len %u\n", max_len);

    XLOG_ASSERT(d != 0);	// this is a programming error
    if (max_len < 2)
	xorp_throw(CorruptMessage, "Short block to BGPParameter::create\n",
                       OPENMSGERROR, 0);

    ParamType param_type = (ParamType)d[0];
    len = d[1] + 2;	// count the header too

    if (len == 2 || len > max_len ) {
	debug_msg("Badly constructed Parameters\n");
	debug_msg("Throw exception\n");
	debug_msg("Send bad packet\n");
	// XXX there doesn't seem to be a good error code for this.
	xorp_throw(CorruptMessage, "Badly constructed Parameters\n",
		   OPENMSGERROR, 0);
    }
    debug_msg("param type %d len+header %u\n", param_type, (uint32_t)len);

    BGPParameter *p = NULL;
    switch (param_type) {
    case PARAMTYPEAUTH:
	p = new BGPAuthParameter(len, d);
	break;

    case PARAMTYPECAP: {
	CapType cap_type = (CapType)d[2];
	switch (cap_type) { // This is the capability type
	case CAPABILITYMULTIPROTOCOL:
	    p = new BGPMultiProtocolCapability(len, d);
	    break;

	case CAPABILITYREFRESH:
	case CAPABILITYREFRESHOLD:
	    p = new BGPRefreshCapability(len, d);
	    break;

	default:
	    p = new BGPUnknownCapability(len, d);
	    // abort();
	}
	break;
    }

    default :
	debug_msg("Some other type\n");
	debug_msg("Throw exception\n");
	debug_msg("Send invalid packet");
	xorp_throw(CorruptMessage,
	       c_format("Unrecognised optional parameter %d max_len %u len %u",
			param_type, max_len, (uint32_t)len),
	       OPENMSGERROR, UNSUPOPTPAR);
    }
    return p;
}
