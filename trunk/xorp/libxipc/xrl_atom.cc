// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$Id$"

#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#elif  HAVE_SYS_ETHERNET_H
#include <sys/ethernet.h>
#endif

#include <map>
#include <string>

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/xlog.h"

#include "xrl_atom.hh"
#include "xrl_atom_encoding.hh"
#include "xrl_tokens.hh"

// ----------------------------------------------------------------------------
// XrlAtomType-to-name mapping

static const char* xrlatom_no_type_name	= "none";
static const char* xrlatom_boolean_name	= "bool";
static const char* xrlatom_int32_name	= "i32";
static const char* xrlatom_uint32_name	= "u32";
static const char* xrlatom_ipv4_name	= "ipv4";
static const char* xrlatom_ipv4net_name	= "ipv4net";
static const char* xrlatom_ipv6_name	= "ipv6";
static const char* xrlatom_ipv6net_name	= "ipv6net";
static const char* xrlatom_mac_name	= "mac";
static const char* xrlatom_text_name	= "txt";
static const char* xrlatom_list_name	= "list";
static const char* xrlatom_binary_name	= "binary";

const char*
xrlatom_type_name(const XrlAtomType& t)
{
    switch (t) {
#define NAME_CASE(x) case (x): return x##_name
	NAME_CASE(xrlatom_no_type);
	NAME_CASE(xrlatom_boolean);
	NAME_CASE(xrlatom_int32);
	NAME_CASE(xrlatom_uint32);
	NAME_CASE(xrlatom_ipv4);
	NAME_CASE(xrlatom_ipv4net);
	NAME_CASE(xrlatom_ipv6);
	NAME_CASE(xrlatom_ipv6net);
	NAME_CASE(xrlatom_mac);
	NAME_CASE(xrlatom_text);
	NAME_CASE(xrlatom_list);
	NAME_CASE(xrlatom_binary);
	// ... Your type here ...
    }
    return xrlatom_no_type_name;
}

static XrlAtomType
resolve_xrlatom_name(const char* name)
{
    // This awkward construct ensures names of all atoms defined are mapped
    // since it generates a compile time error for missing enum values in
    // XrlAtomType.
    for (XrlAtomType t = xrlatom_start; t <= xrlatom_end;
	 t = XrlAtomType(t + 1)) {
	switch (t) {
#define CHECK_NAME(x) case (x) : if (strcmp(name, x##_name) == 0) return x;
	    CHECK_NAME(xrlatom_int32);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_uint32);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_ipv4);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_ipv4net);	/* FALLTHRU */
	    CHECK_NAME(xrlatom_ipv6);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_ipv6net);	/* FALLTHRU */
	    CHECK_NAME(xrlatom_mac);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_text);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_list);		/* FALLTHRU */
	    CHECK_NAME(xrlatom_boolean);	/* FALLTHRU */
	    CHECK_NAME(xrlatom_binary);		/* FALLTHRU */
	    // ... Your type here ...
	case xrlatom_no_type:
	    break;
	}
	break;
    }
    return xrlatom_no_type;
}

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

ssize_t
XrlAtom::data_from_c_str(const char* c_str)
{
    // Handle binary data type differently to avoid unnecessary copying.
    if (_type == xrlatom_binary) {
	_binary = new vector<uint8_t>();
	ssize_t bad_pos = xrlatom_decode_value(c_str, strlen(c_str), *_binary);
	if (bad_pos >= 0) {
	    delete _binary;
	    xorp_throw0(InvalidString);
	}
	_have_data = true;
	return -1;
    }

    string decoded;
    ssize_t bad_pos = xrlatom_decode_value(c_str, strlen(c_str), decoded);
    if (bad_pos >= 0) {
	xorp_throw0(InvalidString);
    }
    c_str = decoded.c_str();
    _have_data = true;

    switch (_type) {
    case xrlatom_no_type:
	break;
    case xrlatom_boolean:
	_boolean = ('t' == c_str[0] || 'T' == c_str[0] || '1' == c_str[0]);
	break;
    case xrlatom_int32:
	_i32val = (int32_t)strtol(c_str, (char**)NULL, 10);
	break;
    case xrlatom_uint32:
	_u32val = (uint32_t)strtoul(c_str, (char**)NULL, 10);
	break;
    case xrlatom_ipv4:
	_ipv4 = new IPv4(c_str);
	break;
    case xrlatom_ipv4net:
	_ipv4net = new IPv4Net(c_str);
	break;
    case xrlatom_ipv6:
	_ipv6 = new IPv6(c_str);
	break;
    case xrlatom_ipv6net:
	_ipv6net = new IPv6Net(c_str);
	break;
    case xrlatom_mac:
	_mac = new Mac(c_str);
	break;
    case xrlatom_text:
	_text = new string(decoded);
	break;
    case xrlatom_list:
	_list = new XrlAtomList(c_str);
	break;
    case xrlatom_binary:
	abort(); // Binary is a special case and handled at start of routine
	break;

	// ... Your types instantiator here ...
    }
    return -1;
}

XrlAtom::~XrlAtom()
{
    discard_dynamic();
}

// ----------------------------------------------------------------------------
// XrlAtom accessor functions

inline void
XrlAtom::type_and_data_okay(const XrlAtomType& t) const
    throw (NoData, WrongType) {
    if (_type != t)
        xorp_throw(WrongType, t, _type);
    if (_have_data == false)
        xorp_throw0(NoData);
}

const bool&
XrlAtom::boolean() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_boolean);
    return _boolean;
}

const int32_t&
XrlAtom::int32() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_int32);
    return _i32val;
}

const uint32_t&
XrlAtom::uint32() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_uint32);
    return _u32val;
}

const IPv4&
XrlAtom::ipv4() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_ipv4);
    return *_ipv4;
}

const IPv4Net&
XrlAtom::ipv4net() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_ipv4net);
    return *_ipv4net;
}

const IPv6&
XrlAtom::ipv6() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_ipv6);
    return *_ipv6;
}

const IPv6Net&
XrlAtom::ipv6net() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_ipv6net);
    return *_ipv6net;
}

const IPvX
XrlAtom::ipvx() const throw (NoData, WrongType)
{
    if (_type == xrlatom_ipv4) {
	return ipv4();
    } else {
	assert(_type == xrlatom_ipv6);
	return ipv6();
    }
}

const IPvXNet
XrlAtom::ipvxnet() const throw (NoData, WrongType)
{
    if (_type == xrlatom_ipv4net) {
	return ipv4net();
    } else {
	assert(_type == xrlatom_ipv6);
	return ipv6net();
    }
}

const Mac&
XrlAtom::mac() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_mac);
    return *_mac;
}

const string&
XrlAtom::text() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_text);
    return *_text;
}

const XrlAtomList&
XrlAtom::list() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_list);
    return *_list;
}

const vector<uint8_t>&
XrlAtom::binary() const throw (NoData, WrongType)
{
    type_and_data_okay(xrlatom_binary);
    return *_binary;
}

// ----------------------------------------------------------------------------
// XrlAtom dynamic data management functions

void
XrlAtom::copy(const XrlAtom& xa)
{

    set_name(xa.name().c_str());
    _type = xa._type;
    _have_data = xa._have_data;

    if (_have_data) {
        switch (_type) {
	case xrlatom_boolean:
	    _boolean = xa._boolean;
	    break;
        case xrlatom_int32:
            _i32val = xa._i32val;
            break;
        case xrlatom_uint32:
            _u32val = xa._u32val;
            break;
        case xrlatom_ipv4:
            _ipv4 = new IPv4(*xa._ipv4);
            break;
        case xrlatom_ipv4net:
            _ipv4net = new IPv4Net(*xa._ipv4net);
            break;
        case xrlatom_ipv6:
            _ipv6 = new IPv6(*xa._ipv6);
            break;
        case xrlatom_ipv6net:
            _ipv6net = new IPv6Net(*xa._ipv6net);
            break;
        case xrlatom_mac:
            _mac = new Mac(*xa._mac);
            break;
        case xrlatom_text:
            _text = new string(*xa._text);
            break;
	case xrlatom_list:
	    _list = new XrlAtomList(*xa._list);
	    break;
	case xrlatom_binary:
	    _binary = new vector<uint8_t>(*xa._binary);
	    break;

            // ... Your type's copy operation here ...
        case xrlatom_no_type:
            break;
        }
    }
}

inline void
XrlAtom::discard_dynamic()
{
    if (_have_data) {
        switch (_type) {
        case xrlatom_no_type:
	case xrlatom_boolean:
        case xrlatom_int32:
        case xrlatom_uint32:
	    break;
        case xrlatom_ipv4:
            delete _ipv4;
            _ipv4 = 0;
            break;
        case xrlatom_ipv4net:
            delete _ipv4net;
            _ipv4net = 0;
            break;
        case xrlatom_ipv6:
            delete _ipv6;
            _ipv6 = 0;
            break;
        case xrlatom_ipv6net:
            delete _ipv6net;
            _ipv6net = 0;
            break;
        case xrlatom_mac:
            delete _mac;
            _mac = 0;
            break;
        case xrlatom_text:
            delete _text;
            _text = 0;
	    break;
	case xrlatom_list:
	    delete _list;
	    _list = 0;
            break;
	case xrlatom_binary:
	    delete _binary;
	    _binary = 0;
	    break;

            // ... Your type should free allocated memory here ...
        }
        _have_data = false;
    }
}

// ----------------------------------------------------------------------------
// XrlAtom save / restore string representations
// See devnotes/xrl-syntax.txt for more info.

string
XrlAtom::str() const
{
    if (_have_data) {
	return c_format("%s%s%s%s%s", name().c_str(), XrlToken::ARG_NT_SEP,
			type_name().c_str(), XrlToken::ARG_TV_SEP,
			value().c_str());
    }
    return c_format("%s%s%s", name().c_str(), XrlToken::ARG_NT_SEP,
		    type_name().c_str());
}

XrlAtom::XrlAtom(const char* serialized) throw (InvalidString, BadName)
    : _type(xrlatom_no_type), _have_data(false)
{

    const char *start, *sep;

    start = serialized;
    // Get Name

    sep = strstr(start, XrlToken::ARG_NT_SEP);
    if (0 == sep) {
	set_name("");
	start = serialized;
    } else {
	set_name(string(start, sep - start));
	start = sep + TOKEN_BYTES(XrlToken::ARG_NT_SEP) - 1;
    }

    // Get Type
    sep = strstr(start, XrlToken::ARG_TV_SEP);
    if (0 == sep) {
	_type = resolve_type_c_str(start);
	_have_data = false;
	if (_type == xrlatom_no_type)
	    xorp_throw(InvalidString,
		       c_format("xrlatom bad type: \"%s\"", start));
    } else {
	_type = resolve_type_c_str(string(start, sep).c_str());
	if (xrlatom_no_type == _type)
	    xorp_throw(InvalidString,
		       c_format("xrlatom bad type: \"%s\"",
				string(start, sep).c_str()));
	start = sep + TOKEN_BYTES(XrlToken::ARG_TV_SEP) - 1;
	// Get Data
	ssize_t bad_pos = data_from_c_str(start);
	if (bad_pos >= 0)
	    xorp_throw0(InvalidString);
    }
}

XrlAtom::XrlAtom(const string& name, XrlAtomType t,
		 const string& serialized_data) throw (InvalidString)
    : _type(t)
{
    set_name(name);
    ssize_t bad_pos = data_from_c_str(serialized_data.c_str());
    if (bad_pos >= 0)
	xorp_throw0(InvalidString);
}

XrlAtom::XrlAtom(const char* name, XrlAtomType t,
		 const string& serialized_data) throw (InvalidString)
    : _type(t)
{
    set_name(name);
    ssize_t bad_pos = data_from_c_str(serialized_data.c_str());
    if (bad_pos >= 0)
	xorp_throw0(InvalidString);
}

const string
XrlAtom::value() const
{
    static char tmp[32];
    tmp[0] = '\0';

    switch (_type) {
    case xrlatom_no_type:
	break;
    case xrlatom_boolean:
	snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]),
		 _boolean ? "true" : "false");
	return xrlatom_encode_value(tmp, strlen(tmp));
	break;
    case xrlatom_int32:
	snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), "%d", _i32val);
	return xrlatom_encode_value(tmp, strlen(tmp));
	break;
    case xrlatom_uint32:
	snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), "%u", _u32val);
	return xrlatom_encode_value(tmp, strlen(tmp));
	break;
    case xrlatom_ipv4:
	return xrlatom_encode_value(_ipv4->str());
    case xrlatom_ipv4net:
	return xrlatom_encode_value(_ipv4net->str());
    case xrlatom_ipv6:
	return xrlatom_encode_value(_ipv6->str());
    case xrlatom_ipv6net:
	return xrlatom_encode_value(_ipv6net->str());
    case xrlatom_mac:
	return xrlatom_encode_value(_mac->str());
    case xrlatom_text:
	return xrlatom_encode_value(*_text);
    case xrlatom_list:
	return _list->str();
    case xrlatom_binary:
	return xrlatom_encode_value(*_binary);

	// ... Your type's c_str equivalent here ...
    }

    return tmp;
}

const string
XrlAtom::type_name() const
{
    return xrlatom_type_name(_type);
}

XrlAtomType
XrlAtom::resolve_type_c_str(const char *c_str)
{
    return resolve_xrlatom_name(c_str);
}

// ----------------------------------------------------------------------------
// Misc Operators

bool
XrlAtom::operator==(const XrlAtom& other) const
{

    bool mn = (name() == other.name());
    bool mt = (_type == other._type);
    bool md = (_have_data == other._have_data);
    bool mv = true;
    if (_have_data && md) {
	switch (_type) {
	case xrlatom_no_type:
	    mv = true;
	    break;
	case xrlatom_boolean:
	    mv = (_boolean == other._boolean);
	    break;
	case xrlatom_int32:
	    mv = (_i32val == other._i32val);
	    break;
	case xrlatom_uint32:
	    mv = (_u32val == other._u32val);
	    break;
	case xrlatom_ipv4:
	    mv = (*_ipv4 == *other._ipv4);
	    break;
	case xrlatom_ipv4net:
	    mv = (*_ipv4net == *other._ipv4net);
	    break;
	case xrlatom_ipv6:
	    mv = (*_ipv6 == *other._ipv6);
	    break;
	case xrlatom_ipv6net:
	    mv = (*_ipv6net == *other._ipv6net);
	    break;
	case xrlatom_mac:
	    mv = (*_mac == *other._mac);
	    break;
	case xrlatom_text:
	    mv = (*_text == *other._text);
	    break;
	case xrlatom_list:
	    mv = (*_list == *other._list);
	    break;
	case xrlatom_binary:
	    mv = (*_binary == *other._binary);
	    break;

	    // ... Your type's equality test here ...
	}
    }
    //    debug_msg("%d%d%d%d\n", mn, mt, md, mv);
    return mn == mt == md == mv; // redundant return value
}

// ----------------------------------------------------------------------------
// Binary Packing

// Packing header is 8-bits:
// hdr[7:7] _have_name
// hdr[6:6] _have_data
// hdr[5:0] _type number

#define NAME_PRESENT 0x80
#define DATA_PRESENT 0x40

// if _have_name then the name is packed as 16-bit unsigned number
// indicating the length, then the ascii representation of the name

// if _have_data then data follows.  Data is packed in
// network order where appropriate.  The only "funny" data type is
// xrlatom_text which we prefix with a 32-bit length field.

size_t
XrlAtom::packed_bytes() const
{
    size_t bytes = 1;	// Packing header space

    if (name().size() > 0) {
	bytes += 2 + name().size();
    }

    if (!_have_data) {
	return bytes;
    }

    static_assert(sizeof(IPv4) == 4);
    static_assert(sizeof(IPv6) == 16);
    static_assert(sizeof(IPv4Net) == sizeof(IPv4) + 4);
    static_assert(sizeof(IPv6Net) == sizeof(IPv6) + 4);

    switch (_type) {
    case xrlatom_no_type:
	break;
    case xrlatom_boolean:
	bytes += 1; // A crime
	break;
    case xrlatom_int32:
    case xrlatom_uint32:
    case xrlatom_ipv4:
	bytes += 4;
	break;
    case xrlatom_ipv4net:
	bytes += 5;
	break;
    case xrlatom_ipv6:
	bytes += 16;
	break;
    case xrlatom_ipv6net:
	bytes += 17;
	break;
    case xrlatom_mac:
	bytes += 4;
	bytes += _mac->str().size();
	break;
    case xrlatom_text:
	bytes += 4;
	bytes += _text->size();
	break;
    case xrlatom_list:
	bytes += 4;
	for (size_t i = 0; i < _list->size(); i++)
	    bytes += _list->get(i).packed_bytes();
	break;
    case xrlatom_binary:
	assert(_binary != 0);
	bytes += 4 + _binary->size();
	break;

	// ... Your type sizing operation here ...
    }
    return bytes;
}

/** @return true if packed size of atom type is fixed, false if depends
 *  packed size depends on data content (eg list, text, binary)
 */
inline bool
XrlAtom::packed_bytes_fixed() const
{
    switch (_type) {
    case xrlatom_no_type:
    case xrlatom_int32:
    case xrlatom_uint32:
    case xrlatom_ipv4:
    case xrlatom_ipv4net:
    case xrlatom_ipv6:
    case xrlatom_ipv6net:
    case xrlatom_boolean:
	return true;
    case xrlatom_mac:
    case xrlatom_text:
    case xrlatom_list:
    case xrlatom_binary:
	return false;
    }
    return false;
}

size_t
XrlAtom::pack_name(uint8_t* buffer) const
{
    assert(name().size() > 0 && name().size() < 65536);
    uint16_t sz = (uint16_t)name().size();
    buffer[0] = sz >> 8;
    buffer[1] = sz & 0xff;
    memcpy(buffer + sizeof(sz), name().c_str(), name().size());
    return sizeof(sz) + sz;
}

size_t
XrlAtom::unpack_name(const uint8_t* buffer, size_t buffer_bytes)
    throw (BadName)
{
    uint16_t sz;
    if (buffer_bytes < sizeof(sz)) {
	return 0;
    }
    sz = (buffer[0] << 8) | buffer[1];
    if (buffer_bytes < sz) {
	return 0;
    }
    const char* s = reinterpret_cast<const char*>(buffer + sizeof(sz));
    set_name(string(s, sz).c_str());
    return sizeof(sz) + sz;
}

size_t
XrlAtom::pack_boolean(uint8_t* buffer) const
{
    buffer[0] = (uint8_t)_boolean;
    return 1;
}

size_t
XrlAtom::unpack_boolean(const uint8_t* buf)
{
    _boolean = bool(buf[0]);
    return 1;
}

size_t
XrlAtom::pack_uint32(uint8_t* buffer) const
{
    buffer[0] = (uint8_t)(_u32val >> 24);
    buffer[1] = (uint8_t)(_u32val >> 16);
    buffer[2] = (uint8_t)(_u32val >> 8);
    buffer[3] = (uint8_t)(_u32val);
    return sizeof(_u32val);
}

size_t
XrlAtom::unpack_uint32(const uint8_t* buf)
{
    _u32val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    return sizeof(_u32val);
}

size_t
XrlAtom::pack_ipv4(uint8_t* buffer) const
{
    uint32_t ul = _ipv4->addr();
    memcpy(buffer, &ul, sizeof(ul));
    return sizeof(ul);
}

size_t
XrlAtom::unpack_ipv4(const uint8_t* b)
{
    uint32_t a;
    memcpy(&a, b, sizeof(a));
    _ipv4 = new IPv4(a);
    return sizeof(a);
}

size_t
XrlAtom::pack_ipv4net(uint8_t* buffer) const
{
    uint32_t ul = _ipv4net->masked_addr().addr();
    memcpy(buffer, &ul, sizeof(ul));
    buffer[sizeof(ul)] = (uint8_t)_ipv4net->prefix_len();
    return sizeof(ul) + sizeof(uint8_t);
}

size_t
XrlAtom::unpack_ipv4net(const uint8_t* b)
{
    uint32_t a;
    memcpy(&a, b, sizeof(a));
    IPv4 v(a);
    _ipv4net = new IPv4Net(v, b[sizeof(a)]);
    return sizeof(a) + sizeof(uint8_t);
}

size_t
XrlAtom::pack_ipv6(uint8_t* buffer) const
{
    const uint32_t* a = _ipv6->addr();
    memcpy(buffer, a, 4 * sizeof(*a));
    return 4 * sizeof(*a);
}

size_t
XrlAtom::unpack_ipv6(const uint8_t* buffer)
{
    uint32_t a[4];
    memcpy(a, buffer, sizeof(a));
    _ipv6 = new IPv6(a);
    return sizeof(a);
}

size_t
XrlAtom::pack_ipv6net(uint8_t* buffer) const
{
    const uint32_t* a = _ipv6net->masked_addr().addr();
    memcpy(buffer, a, 4 * sizeof(*a));
    buffer[sizeof(IPv6)] = (uint8_t)_ipv6net->prefix_len();
    return 4 * sizeof(*a) + sizeof(uint8_t);
}

size_t
XrlAtom::unpack_ipv6net(const uint8_t* buffer)
{
    uint32_t a[4];
    memcpy(a, buffer, sizeof(a));
    IPv6 v(a);
    _ipv6net = new IPv6Net(v, buffer[sizeof(a)]);
    return sizeof(a) + sizeof(uint8_t);
}

size_t
XrlAtom::pack_mac(uint8_t* buffer) const
{
    string ser = _mac->str();
    uint32_t sz = ser.size();
    uint32_t ul = htonl(sz);
    memcpy(buffer, &ul, sizeof(ul));
    if (sz > 0) {
	memcpy(buffer + sizeof(ul), ser.c_str(), sz);
    }
    return sizeof(ul) + sz;
}

size_t
XrlAtom::unpack_mac(const uint8_t* buffer, size_t buffer_bytes)
{
    if (buffer_bytes < sizeof(uint32_t)) {
	return 0;
    }
    uint32_t len;
    memcpy(&len, buffer, sizeof(len));
    len = ntohl(len);
    if (buffer_bytes < (len + sizeof(len))) {
	return 0;
    }
    const char *text = reinterpret_cast<const char*>(buffer + sizeof(len));
    _mac = new Mac(string(text, len));
    return sizeof(len) + len;
}

size_t
XrlAtom::pack_text(uint8_t* buffer) const
{
    uint32_t sz = _text->size();
    uint32_t ul = htonl(sz);
    memcpy(buffer, &ul, sizeof(ul));
    if (sz > 0) {
	memcpy(buffer + sizeof(ul), _text->c_str(), sz);
    }
    return sizeof(ul) + sz;
}

size_t
XrlAtom::unpack_text(const uint8_t* buffer, size_t buffer_bytes)
{
    if (buffer_bytes < sizeof(uint32_t)) {
	return 0;
    }
    uint32_t len;
    memcpy(&len, buffer, sizeof(len));
    len = ntohl(len);
    if (buffer_bytes < (len + sizeof(len))) {
	return 0;
    }
    const char *text = reinterpret_cast<const char*>(buffer + sizeof(len));
    _text = new string(text, len);
    return sizeof(len) + len;
}

size_t
XrlAtom::pack_list(uint8_t* buffer, size_t buffer_bytes) const
{
    size_t done = 0;

    size_t nelem = htonl(_list->size());
    static_assert(sizeof(nelem) == 4);

    memcpy(buffer, &nelem, sizeof(nelem));
    done += sizeof(nelem);

    nelem = ntohl(nelem);
    for (size_t i = 0; i < nelem; i++) {
	done += _list->get(i).pack(buffer + done, buffer_bytes - done);
	assert(done <= buffer_bytes);
    }
    return done;
}

size_t
XrlAtom::unpack_list(const uint8_t* buffer, size_t buffer_bytes)
{
    size_t used = 0;
    size_t nelem;

    memcpy(&nelem, buffer, sizeof(nelem));
    used += sizeof(nelem);

    _list = new XrlAtomList;

    nelem = ntohl(nelem);
    for (size_t i = 0; i < nelem; i++) {
	XrlAtom tmp;
	used += tmp.unpack(buffer + used, buffer_bytes - used);
	assert(used <= buffer_bytes);
	_list->append(tmp);
    }
    return used;
}

size_t
XrlAtom::pack_binary(uint8_t* buffer) const
{
    uint32_t sz = _binary->size();
    uint32_t ul = htonl(sz);
    memcpy(buffer, &ul, sizeof(ul));
    if (sz > 0) {
	memcpy(buffer + sizeof(ul), _binary->begin(), sz);
    }
    return sizeof(ul) + sz;
}

size_t
XrlAtom::unpack_binary(const uint8_t* buffer, size_t buffer_bytes)
{
    if (buffer_bytes < sizeof(uint32_t)) {
	return 0;
    }
    uint32_t len;
    memcpy(&len, buffer, sizeof(len));
    len = ntohl(len);
    if (buffer_bytes < (len + sizeof(len))) {
	abort();
	return 0;
    }
    _binary = new vector<uint8_t>(buffer + sizeof(len),
				  buffer + sizeof(len) + len);
    return sizeof(len) + len;
}

size_t
XrlAtom::pack(uint8_t* buffer, size_t buffer_bytes) const
{
    if (buffer_bytes < packed_bytes()) {
	debug_msg("Buffer too small (%d < %d)\n",
		  buffer_bytes, packed_bytes());
	return 0;
    }

    uint8_t& header = *buffer;
    header = _type;

    size_t packed_size = 1;

    if (name().size()) {
	header |= NAME_PRESENT;
	packed_size += pack_name(buffer + packed_size);
    }

    if (_have_data) {
	header |= DATA_PRESENT;
	switch (_type) {
	case xrlatom_no_type:
	    abort();
	case xrlatom_boolean:
	    packed_size += pack_boolean(buffer + packed_size);
	    break;
	case xrlatom_int32:
	case xrlatom_uint32:
	    packed_size += pack_uint32(buffer + packed_size);
	    break;
	case xrlatom_ipv4:
	    packed_size += pack_ipv4(buffer + packed_size);
	    break;
	case xrlatom_ipv4net:
	    packed_size += pack_ipv4net(buffer + packed_size);
	    break;
	case xrlatom_ipv6:
	    packed_size += pack_ipv6(buffer + packed_size);
	    break;
	case xrlatom_ipv6net:
	    packed_size += pack_ipv6net(buffer + packed_size);
	    break;
	case xrlatom_mac:
	    packed_size += pack_mac(buffer + packed_size);
	    break;
	case xrlatom_text:
	    packed_size += pack_text(buffer + packed_size);
	    break;
	case xrlatom_list:
	    packed_size += pack_list(buffer + packed_size,
				     buffer_bytes - packed_size);
	    break;
	case xrlatom_binary:
	    packed_size += pack_binary(buffer + packed_size);
	    break;

	    // ... Your type here ...
	}
    }
    return packed_size;
}

size_t
XrlAtom::unpack(const uint8_t* buffer, size_t buffer_bytes)
{
    if (buffer_bytes == 0) {
	debug_msg("Shoot! Passed 0 length buffer for unpacking\n");
	return 0;
    }

    const uint8_t& header = *buffer;
    size_t unpacked = 1;

    if (header & NAME_PRESENT) {
	try {
	    size_t used = unpack_name(buffer + unpacked, buffer_bytes - unpacked);
	    unpacked += used;
	} catch (const XrlAtom::BadName& bn) {
	    debug_msg("Unpacking failed:\n%s\n", bn.str().c_str());
	    return 0;
	}
    }

    if (header & DATA_PRESENT) {
	int t = header & ~(NAME_PRESENT | DATA_PRESENT);
	if (t < xrlatom_start || t > xrlatom_end) {
	    debug_msg("Type %d invalid\n", t);
	}
	_type = XrlAtomType(t);
	_have_data = true;

	// Check size for fixed width packed types.
	if (packed_bytes_fixed() && buffer_bytes < packed_bytes()) {
	    debug_msg("Insufficient space (%d < %d) for type %d\n",
		      buffer_bytes - unpacked, packed_bytes(), _type);
	    return 0;
	}

	// unpack
	size_t used = 0;
	switch (_type) {
	case xrlatom_no_type:
	    return 0;
	case xrlatom_boolean:
	    used = unpack_boolean(buffer + unpacked);
	    break;
	case xrlatom_int32:
	case xrlatom_uint32:
	    used = unpack_uint32(buffer + unpacked);
	    break;
	case xrlatom_ipv4:
	    used = unpack_ipv4(buffer + unpacked);
	    break;
	case xrlatom_ipv4net:
	    used = unpack_ipv4net(buffer + unpacked);
	    break;
	case xrlatom_ipv6:
	    used = unpack_ipv6(buffer + unpacked);
	    break;
	case xrlatom_ipv6net:
	    used = unpack_ipv6net(buffer + unpacked);
	    break;
	case xrlatom_mac:
	    used = unpack_mac(buffer + unpacked, buffer_bytes - unpacked);
	    break;
	case xrlatom_text:
	    used = unpack_text(buffer + unpacked, buffer_bytes - unpacked);
	    break;
	case xrlatom_list:
	    used = unpack_list(buffer + unpacked, buffer_bytes - unpacked);
	    break;
	case xrlatom_binary:
	    used = unpack_binary(buffer + unpacked, buffer_bytes - unpacked);
	    break;

	    // ... Your type here ...
	}

	if (used == 0) {
	    debug_msg("Blow! Data unpacking failed\n");
	    return 0;
	}
	unpacked += used;
    }
    return unpacked;
}

void
XrlAtom::set_name(const char *name) throw (BadName)
{
    if (name == 0) name = "";
    _atom_name = name;
    if (!valid_name(name))
	xorp_throw(BadName, name);
}

bool
XrlAtom::valid_name(const string& s)
{
    string::const_iterator si;
    si = s.begin();
    while (si != s.end()) {
	if (*si != '_' && *si != '-' && !isalnum(*si))
	    return false;
	si++;
    }
    return true;
}

bool
XrlAtom::valid_type(const string& s)
{
    XrlAtomType t = resolve_xrlatom_name(s.c_str());
    return (t != xrlatom_no_type);
}
