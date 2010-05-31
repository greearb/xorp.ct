// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libproto/packet.hh"

#include <functional>
#include <stdexcept>

#include "xrl_args.hh"
#include "xrl_tokens.hh"

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove XrlAtom

XrlArgs&
XrlArgs::add(const XrlAtom& xa) throw (XrlAtomFound)
{
    if (!xa.name().empty()) {
	const_iterator p;

	for (p = _args.begin(); p != _args.end(); ++p) {
	    if (p->name() == xa.name()) {
		throw XrlAtomFound();
	    }
	}

	_have_name = true;
    }

    _args.push_back(xa);
    return *this;
}

const XrlAtom&
XrlArgs::get(const XrlAtom& dataless) const throw (XrlAtomNotFound)
{
    const_iterator p;
    for (p = _args.begin(); p != _args.end(); ++p) {
	if (p->type() == dataless.type() &&
	    p->name() == dataless.name()) {
	    return *p;
	}
    }
    throw XrlAtomNotFound();
    return *p;
}

const XrlAtom&
XrlArgs::get(unsigned idx, const char* name) const throw (XrlAtomNotFound)
{
    if (!_have_name)
	return _args[idx];

    for (const_iterator i = _args.begin(); i != _args.end(); ++i) {
	const XrlAtom& a = *i;

	if (a.name().compare(name) == 0)
	    return a;
    }

    throw XrlAtomNotFound();
}

void
XrlArgs::remove(const XrlAtom& dataless) throw (XrlAtomNotFound)
{
    iterator p;
    for (p = _args.begin(); p != _args.end(); ++p) {
	if (p->type() == dataless.type() &&
	    p->name() == dataless.name()) {
	    _args.erase(p);
	    return;
	    }
    }
    throw XrlAtomNotFound();
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove bool

XrlArgs&
XrlArgs::add_bool(const char* name, bool val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const bool&
XrlArgs::get_bool(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_boolean)).boolean();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_bool(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_boolean));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove int32

XrlArgs&
XrlArgs::add_int32(const char* name, int32_t val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const int32_t&
XrlArgs::get_int32(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_int32)).int32();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_int32(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_int32));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove uint32

XrlArgs&
XrlArgs::add_uint32(const char* name, uint32_t val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const uint32_t&
XrlArgs::get_uint32(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_uint32)).uint32();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_uint32(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_uint32));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv4

XrlArgs&
XrlArgs::add_ipv4(const char* name, const IPv4& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPv4&
XrlArgs::get_ipv4(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv4)).ipv4();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_ipv4(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_ipv4));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv4net

XrlArgs&
XrlArgs::add_ipv4net(const char* name, const IPv4Net& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPv4Net&
XrlArgs::get_ipv4net(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv4net)).ipv4net();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_ipv4net(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_ipv4net));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv6

XrlArgs&
XrlArgs::add_ipv6(const char* name, const IPv6& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPv6&
XrlArgs::get_ipv6(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv6)).ipv6();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_ipv6(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_ipv6));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv6net

XrlArgs&
XrlArgs::add_ipv6net(const char* name, const IPv6Net& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPv6Net&
XrlArgs::get_ipv6net(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv6net)).ipv6net();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_ipv6net(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_ipv6net));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipvx - not there is no underlying XrlAtom type
// for IPvX.  It is just a wrapper for IPv4 and IPv6 and provided as
// a convenience.

XrlArgs&
XrlArgs::add_ipvx(const char* name, const IPvX& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPvX
XrlArgs::get_ipvx(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv4)).ipv4();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType&) {
	try {
	    return get(XrlAtom(name, xrlatom_ipv6)).ipv6();
	} catch (const XrlAtom::WrongType& e) {
	    xorp_throw(BadArgs, e.why());
	}
    }
    xorp_throw(BadArgs, c_format("Unknown error for atom name %s", name));
    XLOG_UNREACHABLE();
}

void
XrlArgs::remove_ipvx(const char* name) throw (XrlAtomNotFound)
{
    try {
	remove(XrlAtom(name, xrlatom_ipv4));
    } catch (const XrlAtomNotFound&) {
	remove(XrlAtom(name, xrlatom_ipv6));
    }
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipvxnet - not there is no underlying XrlAtom
// type for IPvXNet.  It is just a wrapper for IPv4Net and IPv6Net and
// provided as a convenience.

XrlArgs&
XrlArgs::add_ipvxnet(const char* name, const IPvXNet& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const IPvXNet
XrlArgs::get_ipvxnet(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_ipv4net)).ipv4net();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType&) {
	try {
	    return get(XrlAtom(name, xrlatom_ipv6net)).ipv6net();
	} catch (const XrlAtom::WrongType& e) {
	    xorp_throw(BadArgs, e.why());
	}
    }
    xorp_throw(BadArgs, c_format("Unknown error for atom name %s", name));
    XLOG_UNREACHABLE();
}

void
XrlArgs::remove_ipvxnet(const char* name) throw (XrlAtomNotFound)
{
    try {
	remove(XrlAtom(name, xrlatom_ipv4net));
    } catch (const XrlAtomNotFound&) {
	remove(XrlAtom(name, xrlatom_ipv6net));
    }
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove mac

XrlArgs&
XrlArgs::add_mac(const char* name, const Mac& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const Mac&
XrlArgs::get_mac(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_mac)).mac();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_mac(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_mac));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove string

XrlArgs&
XrlArgs::add_string(const char* name, const string& val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const string&
XrlArgs::get_string(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_text)).text();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_string(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_text));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove list

XrlArgs&
XrlArgs::add_list(const char* name, const XrlAtomList& val)
    throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const XrlAtomList&
XrlArgs::get_list(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_list)).list();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_list(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_list));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove binary data

XrlArgs&
XrlArgs::add_binary(const char* name, const vector<uint8_t>& val)
    throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const vector<uint8_t>&
XrlArgs::get_binary(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_binary)).binary();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_binary(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_binary));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove int64

XrlArgs&
XrlArgs::add_int64(const char* name, int64_t val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const int64_t&
XrlArgs::get_int64(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_int64)).int64();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_int64(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_int64));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove uint64

XrlArgs&
XrlArgs::add_uint64(const char* name, uint64_t val) throw (XrlAtomFound)
{
    return add(XrlAtom(name, val));
}

const uint64_t&
XrlArgs::get_uint64(const char* name) const throw (BadArgs)
{
    try {
	return get(XrlAtom(name, xrlatom_uint64)).uint64();
    } catch (const XrlAtom::NoData& e) {
        xorp_throw(BadArgs, e.why());
    } catch (const XrlAtom::WrongType& e) {
        xorp_throw(BadArgs, e.why());
    }
}

void
XrlArgs::remove_uint64(const char* name) throw (XrlAtomNotFound)
{
    remove(XrlAtom(name, xrlatom_uint64));
}


// ----------------------------------------------------------------------------
// Append an existing XrlArgs

XrlArgs&
XrlArgs::add(const XrlArgs& args) throw (XrlAtomFound)
{
    for (const_iterator ci = args.begin(); ci != args.end(); ci++)
	add(*ci);
    return *this;
}

// ----------------------------------------------------------------------------
// Misc functions

bool
XrlArgs::matches_template(XrlArgs& t) const
{
    if (t._args.size() != _args.size()) {
        return false;
    }

    const_iterator ai = _args.begin();
    const_iterator ti = t._args.begin();
    while (ai != _args.end() &&
           ai->type() == ti->type() &&
           ai->name() != ti->name()) {
        ai++;
        ti++;
    }
    return (ai == _args.end());
}

bool
XrlArgs::operator==(const XrlArgs& t) const
{
    if (t._args.size() != _args.size()) {
        return false;
    }

    const_iterator ai = _args.begin();
    const_iterator ti = t._args.begin();
    while (ai != _args.end() &&
           *ai == *ti) {
        ai++;
        ti++;
    }
    return (ai == _args.end());
}

const XrlAtom&
XrlArgs::operator[](uint32_t index) const
{
    return _args[index];
}

const XrlAtom&
XrlArgs::operator[](const string& name) const throw (XrlAtomNotFound)
{
    for ( const_iterator ai = _args.begin() ; ai != _args.end() ; ai++ ) {
	if (ai->name() == name)
	    return *ai;
    }
    throw XrlAtomNotFound();
    return *(_args.begin()); /* NOT REACHED */
}

size_t
XrlArgs::size() const
{
    return _args.size();
}


// ----------------------------------------------------------------------------
// String serialization methods

string
XrlArgs::str() const
{
    string s;

    const_iterator ai = _args.begin();
    while (ai != _args.end()) {
        s += ai->str();
        ai++;
        if (ai != _args.end())
            s += string(XrlToken::ARG_ARG_SEP);
    }
    return s;
}

XrlArgs::XrlArgs(const char* serialized) throw (InvalidString)
		: _have_name(false)
{
    string s(serialized);

    for (string::iterator start = s.begin(); start < s.end(); start++) {
	string::iterator end = find(start, s.end(), XrlToken::ARG_ARG_SEP[0]);
        string tok(start, end);
	try {
	    XrlAtom xa(tok.c_str());
	    add(xa);
	} catch (const XrlAtomFound& xaf) {
	    xorp_throw(InvalidString, "Duplicate Atom found:" + tok);
	}
	start = end;
    }
}


// ----------------------------------------------------------------------------
// Byte serialization methods

//
// Packed XrlArgs has a 4-byte header:
// MSB is a 1-byte verification value.
// LSB's contain count of arguments expected
//

static const uint32_t PACKING_CHECK_CODE = 0xcc;
static const uint32_t PACKING_MAX_COUNT	 = 0x00ffffff;

size_t
XrlArgs::packed_bytes(XrlAtom* head) const
{
    size_t total_bytes = 0;

    if (head)
	total_bytes += head->packed_bytes();

    for (const_iterator ci = _args.begin(); ci != _args.end(); ++ci) {
	total_bytes += ci->packed_bytes();
    }
    return total_bytes + 4;
}

size_t
XrlArgs::pack(uint8_t* buffer, size_t buffer_bytes, XrlAtom* head) const
{
    size_t total_bytes = 0;

    // Check space exists for header
    if (buffer_bytes < 4) {
	return 0;
    }

    // Pack header
    uint32_t cnt = _args.size();
    if (head)
	cnt++;

    if (cnt > PACKING_MAX_COUNT) {
	// log a message, bounds hit
	return 0;
    }

    uint32_t header = (PACKING_CHECK_CODE << 24) | cnt;
    header = htonl(header);
    memcpy(buffer, &header, sizeof(header));
    total_bytes += sizeof(header);

    if (head) {
	size_t atom_bytes = head->pack(buffer + total_bytes,
				       buffer_bytes - total_bytes);

	if (atom_bytes == 0)
	    return 0;

	total_bytes += atom_bytes;
    }

    // Pack atoms
    for (const_iterator ci = _args.begin(); ci != _args.end(); ++ci) {
	size_t atom_bytes = ci->pack(buffer + total_bytes,
				     buffer_bytes - total_bytes);

	if (atom_bytes == 0) {
	    return 0;
	}
	total_bytes += atom_bytes;
    }
    return total_bytes;
}

size_t
XrlArgs::unpack(const uint8_t* buffer, size_t buffer_bytes, XrlAtom* head)
{
    uint32_t cnt;
    int added = 0;
    size_t used_bytes = unpack_header(cnt, buffer, buffer_bytes);
    XrlAtom* atom;
    _have_name = false;

    if (!used_bytes)
	return 0;

    while (cnt != 0) {
	if (head) {
	    atom = head;
	    head = NULL;
	} else {
	    _args.push_back(XrlAtom());
	    atom = &_args.back();
	    added++;
	}

	size_t atom_bytes = atom->unpack(buffer + used_bytes,
					 buffer_bytes - used_bytes);
	if (atom_bytes == 0)
	    goto __error;

	if (!_have_name && !atom->name().empty())
	    _have_name = true;

	used_bytes += atom_bytes;
	--cnt;
	if (used_bytes >= buffer_bytes) {
	    // Greater than would be bad...
	    assert(used_bytes == buffer_bytes);
	    break;
	}
    }
    if (cnt != 0)
	// Unexpected truncation
	goto __error;

    return used_bytes;

__error:
    while (added--)
	_args.pop_back();

    return 0;
}

size_t
XrlArgs::unpack_header(uint32_t& cnt, const uint8_t* in, size_t len)
{
    // Unpack header
    if (len < 4)
	return 0;

    uint32_t header = extract_32(in);

    // Check header sanity
    if ((header >> 24) != PACKING_CHECK_CODE)
	return 0;

    cnt = header & PACKING_MAX_COUNT;

    return sizeof(header);
}

size_t
XrlArgs::fill(const uint8_t* in, size_t len)
{
    size_t tot = len;
    _have_name = false;

    for (ATOMS::iterator i = _args.begin(); i != _args.end(); ++i) {
	XrlAtom& atom = *i;

	size_t sz = atom.unpack(in, len);
	if (sz == 0)
	    return 0;

	if (!_have_name && !atom.name().empty())
	    _have_name = true;

	XLOG_ASSERT(sz <= len);
	in  += sz;
	len -= sz;
    }

    return tot - len;
}
