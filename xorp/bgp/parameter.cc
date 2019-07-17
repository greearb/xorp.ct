// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libproto/packet.hh"

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
    if (param._data != NULL) {
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

    XLOG_ASSERT(l >= 2 && l < 256);
    debug_msg("_length set %d\n", l);
    _length = l;
    _data = new uint8_t[_length];
    _data[1] = _length-2;
}

/* **************** BGPCapParameter *********************** */

BGPCapParameter::BGPCapParameter()
    : BGPParameter()
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
    debug_msg("_type %d _length %d (total length %d)\n",
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
    /*
    ** Note: In the normal case this method is called by
    ** BGPParameter::create, which has already checked the type,
    ** length and capability fields. It is therefore safe to have
    ** asserts in here as a sanity check.
    */
    _type = static_cast<ParamType>(*_data);
    XLOG_ASSERT(_type == PARAMTYPECAP);

    _length = *(_data+1) + 2; // includes 2 byte header
    XLOG_ASSERT(_length == 4);

    _cap_code = static_cast<CapType>(*(_data+2));
    if (_cap_code == CAPABILITYREFRESHOLD) {
	_old_type_code = true;
	_cap_code = CAPABILITYREFRESH;
    } else {
	_old_type_code = false;
    }
    XLOG_ASSERT(_cap_code == CAPABILITYREFRESH);

    _cap_length = *(_data+3);
    if (_cap_length > 0) {
	debug_msg("Throw exception\n");
	xorp_throw(CorruptMessage, 
		c_format("Refresh Capability length %d is greater than zero.", 
			 _cap_length), OPENMSGERROR, UNSPECIFIED);
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
    s = "BGP Refresh Capability";
    return s;
}



/* ************** BGPMultiProtocolCapability - ****************** */

BGPMultiProtocolCapability::
BGPMultiProtocolCapability(Afi afi, Safi safi)
    : BGPCapParameter()
{
    _cap_code = CAPABILITYMULTIPROTOCOL;
    _length = 8;
    _data = new uint8_t[_length];
    _address_family = afi;
    _subsequent_address_family = safi;
}

BGPMultiProtocolCapability::
BGPMultiProtocolCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGPMultiProtocolCapability(uint8_t, uint8_t*)\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n",
	      _type, _length, _length+2);
}

BGPMultiProtocolCapability::
BGPMultiProtocolCapability(const BGPMultiProtocolCapability& param)
    : BGPCapParameter(param)
{
    _address_family = param._address_family;
    _subsequent_address_family = param._subsequent_address_family;

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
    _type = static_cast<ParamType>(*_data);
    XLOG_ASSERT(_type == PARAMTYPECAP);	// See comment in:
					// BGPRefreshCapability::decode() 

    _length = *(_data+1) + 2; // includes 2 byte header
    _cap_code = static_cast<CapType>(*(_data+2));
    XLOG_ASSERT(_cap_code == CAPABILITYMULTIPROTOCOL);

    _cap_length = *(_data+3);
    uint16_t afi;
    afi = *(_data+4);
    afi <<= 8;
    afi = *(_data+5);
    switch(afi) {
    case AFI_IPV4_VAL:
	_address_family = AFI_IPV4;
	break;
    case AFI_IPV6_VAL:
	_address_family = AFI_IPV6;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("MultiProtocol Capability unrecognised afi %u",
			    afi),
		   OPENMSGERROR, UNSUPOPTPAR);
    }

    debug_msg("address family %d\n", _address_family);
    uint8_t safi = *(_data+7);
    switch(safi) {
    case SAFI_UNICAST_VAL:
	_subsequent_address_family = SAFI_UNICAST;
	break;
    case SAFI_MULTICAST_VAL:
	_subsequent_address_family = SAFI_MULTICAST;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("MultiProtocol Capability unrecognised safi %u",
			    safi),
		   OPENMSGERROR, UNSUPOPTPAR);
    }
}

void
BGPMultiProtocolCapability::encode() const
{
    debug_msg("Encoding a BGP multi protocol capability parameter\n");
    _data[0] = PARAMTYPECAP;
    _data[1] = 6; // parameter length
    _data[2] = CAPABILITYMULTIPROTOCOL;
    _data[3] = 4; // capability length
    uint16_t k = htons(static_cast<uint16_t>(_address_family)); // AFI
    memcpy(&_data[4],&k, 2);
    _data[6] = 0 ; // Reserved
    _data[7] = _subsequent_address_family ; // SAFI
}

// bool BGPMultiProtocolCapability::operator==(const BGPParameter& rhs) const {
bool 
BGPMultiProtocolCapability::compare(const BGPParameter& rhs) const {
    const BGPMultiProtocolCapability *ptr = 
	dynamic_cast<const BGPMultiProtocolCapability *>(&rhs);
    if(!ptr)
	return false;

    if (_address_family != ptr->get_address_family())
	return false;
    if (_subsequent_address_family != 
	ptr->get_subsequent_address_family_id())
	return false;

    return true;
}

string
BGPMultiProtocolCapability::str() const
{
    return c_format("BGP Multiple Protocol Capability AFI = %d SAFI = %d",
		    _address_family, _subsequent_address_family);
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
    debug_msg("BGPMultiRouteCapability(uint8_t, uint8_t*)\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n", 
	      _type, _length, _length+2);
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
    _type = static_cast<ParamType>(*_data);
    XLOG_ASSERT(_type == PARAMTYPECAP);	// See comment in:
					// BGPRefreshCapability::decode()

    _length = *(_data+1) + 2; // includes 2 byte header

    _cap_code = static_cast<CapType>(*(_data+2));
    XLOG_ASSERT(_cap_code == CAPABILITYMULTIROUTE);

    _cap_length = *(_data+3);
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

/* ************** BGP4ByteASCapability - ****************** */
BGP4ByteASCapability::
BGP4ByteASCapability()
{
    _cap_code = CAPABILITY4BYTEAS;
    _length = 8;
    _data = new uint8_t[_length];
}

BGP4ByteASCapability::
BGP4ByteASCapability(uint8_t l, const uint8_t* d)
    : BGPCapParameter(l, d)
{
    debug_msg("BGP4ByteASCapability(uint8_t, uint8_t*)\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n", 
	      _type, _length, _length+2);
}

BGP4ByteASCapability::
BGP4ByteASCapability(const BGP4ByteASCapability& param)
    : BGPCapParameter(param)
{
    _as4 = param._as4;

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

BGP4ByteASCapability::
BGP4ByteASCapability(const AsNum& as)
{
    _as4 = as.as4();
    _length = 8;
    _data = new uint8_t[_length];
    encode();
}

void
BGP4ByteASCapability::decode()
{
    _type = static_cast<ParamType>(*_data);
    XLOG_ASSERT(_type == PARAMTYPECAP);	// See comment in:
					// BGPRefreshCapability::decode()

    _length = extract_8(_data + 1) + 2;	// includes 2 byte header

    _cap_code = static_cast<CapType>(*(_data + 2));
    XLOG_ASSERT(_cap_code == CAPABILITY4BYTEAS);

    _cap_length = extract_8(_data + 3);
    _as4 = extract_32(_data + 4);
}

void
BGP4ByteASCapability::encode() const
{
    debug_msg("Encoding a BGP 4 byte AS capability parameter\n");
    _data[0] = PARAMTYPECAP;
    _data[1] = 6;
    _data[2] = CAPABILITY4BYTEAS;
    _data[3] = 4;
    uint32_t as = htonl(_as4);
    memcpy(&_data[4], &as, 4);
}

string
BGP4ByteASCapability::str() const
{
    return c_format("BGP 4 Byte AS Capability AS = %u", _as4);
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
    debug_msg("BGPUnkownCapability(uint8_t, uint8_t*)\n");
    decode();
    debug_msg("_type %d _length %d (total length %d) \n",
	      _type, _length, _length+2);
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
    _type = static_cast<ParamType>(*_data);
    XLOG_ASSERT(_type == PARAMTYPECAP);	// See comment in:
					// BGPRefreshCapability::decode()

    _length = (uint8_t)*(_data+1) + 2; // includes 2 byte header

    // the original capability code
    _unknown_cap_code = static_cast<CapType>(*(_data+2)); 

    _cap_code = CAPABILITYUNKNOWN; // hard code capability to be UNKNOWN.

    // don't check the capability type, since we already know we
    // don't know what it is.
    // assert(_cap_code == CAPABILITYUNKNOWN);

    _cap_length = *(_data+3);
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
    XLOG_FATAL("we shouldn't ever send an unknown capability\n");
}

BGPParameter *
BGPParameter::create(const uint8_t* d, uint16_t max_len, size_t& len)
	throw(CorruptMessage)
{
    XLOG_ASSERT(d != 0);	// this is a programming error
    if (max_len < 2)
	xorp_throw(CorruptMessage, "Short block to BGPParameter::create\n",
                       OPENMSGERROR, 0);

    ParamType param_type = static_cast<ParamType>(d[0]);
    len = d[1] + 2;	// count the header too

    if (len == 2 || len > max_len ) {
	XLOG_WARNING("Create: max_len %u len %u type: %u\n", max_len, 
		     (uint32_t)len, (uint32_t)param_type);
	debug_msg("Badly constructed Parameters\n");
	debug_msg("Throw exception\n");
	debug_msg("Send bad packet\n");
	// XXX there doesn't seem to be a good error code for this.
	xorp_throw(CorruptMessage, "Badly constructed Parameters\n",
		   OPENMSGERROR, 0);
    }
    debug_msg("param type %d len+header %u\n", param_type,
	      XORP_UINT_CAST(len));

    BGPParameter *p = NULL;
    switch (param_type) {
    case PARAMTYPEAUTH:
	xorp_throw(CorruptMessage,
	       "Deprecated BGP Authentication Parameter received",
	       OPENMSGERROR, UNSUPOPTPAR);

    case PARAMTYPECAP: {
	CapType cap_type = static_cast<CapType>(d[2]);
	switch (cap_type) { // This is the capability type
	case CAPABILITYMULTIPROTOCOL:
	    p = new BGPMultiProtocolCapability(len, d);
	    break;

	case CAPABILITYREFRESH:
	case CAPABILITYREFRESHOLD:
	    p = new BGPRefreshCapability(len, d);
	    break;

	case CAPABILITYMULTIROUTE:
	    p = new BGPMultiRouteCapability(len, d);
	    break;

	case CAPABILITY4BYTEAS:
	    p = new BGP4ByteASCapability(len, d);
	    break;

	default:
	    p = new BGPUnknownCapability(len, d);
	}
	break;
    }

    default :
	xorp_throw(CorruptMessage,
	       c_format("Unrecognised optional parameter %d max_len %u len %u",
			param_type, max_len, XORP_UINT_CAST(len)),
	       OPENMSGERROR, UNSUPOPTPAR);
    }
    return p;
}
