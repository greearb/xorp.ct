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

#include <string.h>
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>

#include "xrl_module.h"
#include "xrl_args.hh"

#include "xrl_tokens.hh"

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove XrlAtom

XrlArgs&
XrlArgs::add(const XrlAtom& xa) throw (XrlAtomFound) {
    const_iterator p = find_if(_args.begin(), _args.end(), 
			       compose1(bind2nd(equal_to<string>(), xa.name()),
					mem_fun_ref(&XrlAtom::name)));
    if (p != _args.end()) {
	throw XrlAtomFound();
    }
    _args.push_back(xa);
    return *this;
}

const XrlAtom& 
XrlArgs::get(const XrlAtom& dataless) const throw (XrlAtomNotFound) {
    const_iterator p = find_if(_args.begin(), _args.end(),
			       compose2(logical_and<bool>(),
					compose1(bind2nd(equal_to<string>(), 
							 dataless.name()),
						 mem_fun_ref(&XrlAtom::name)),
					compose1(bind2nd(equal_to<XrlAtomType>(), 
							 dataless.type()),
						 mem_fun_ref(&XrlAtom::type))));

    if (p == _args.end()) {
	throw XrlAtomNotFound();
    }
    return *p;
}

void
XrlArgs::remove(const XrlAtom& dataless) throw (XrlAtomNotFound) {
    iterator p = find_if(_args.begin(), _args.end(),
			 compose2(logical_and<bool>(),
				  compose1(bind2nd(equal_to<string>(), dataless.name()),
					   mem_fun_ref(&XrlAtom::name)),
				  compose1(bind2nd(equal_to<XrlAtomType>(), dataless.type()),
					   mem_fun_ref(&XrlAtom::type))));

    if (p == _args.end()) {
	throw XrlAtomNotFound();
    }
    _args.erase(p);
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove bool

XrlArgs&
XrlArgs::add_bool(const char* name, bool val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const bool& 
XrlArgs::get_bool(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_boolean)).boolean();
}

void 
XrlArgs::remove_bool(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_boolean));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove int32

XrlArgs&
XrlArgs::add_int32(const char* name, int32_t val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const int32_t& 
XrlArgs::get_int32(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_int32)).int32();
}

void 
XrlArgs::remove_int32(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_int32));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove uint32

XrlArgs&
XrlArgs::add_uint32(const char* name, uint32_t val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const uint32_t& 
XrlArgs::get_uint32(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_uint32)).uint32();
}

void 
XrlArgs::remove_uint32(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_uint32));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv4

XrlArgs&
XrlArgs::add_ipv4(const char* name, const IPv4& val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const IPv4& 
XrlArgs::get_ipv4(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_ipv4)).ipv4();
}

void 
XrlArgs::remove_ipv4(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_ipv4));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv4net

XrlArgs&
XrlArgs::add_ipv4net(const char* name, const IPv4Net& val) 
    throw (XrlAtomFound) 
{
    return add(XrlAtom(name, val));
}

const IPv4Net& 
XrlArgs::get_ipv4net(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_ipv4net)).ipv4net();
}

void 
XrlArgs::remove_ipv4net(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_ipv4net));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv6

XrlArgs&
XrlArgs::add_ipv6(const char* name, const IPv6& val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const IPv6& 
XrlArgs::get_ipv6(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_ipv6)).ipv6();
}

void 
XrlArgs::remove_ipv6(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_ipv6));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipv6net

XrlArgs&
XrlArgs::add_ipv6net(const char* name, const IPv6Net& val) 
    throw (XrlAtomFound) 
{
    return add(XrlAtom(name, val));
}

const IPv6Net& 
XrlArgs::get_ipv6net(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_ipv6net)).ipv6net();
}

void 
XrlArgs::remove_ipv6net(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_ipv6net));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove ipvx - not there is no underlying XrlAtom type
// for IPvX.  It is just a wrapper for IPv4 and IPv6 and provided as
// a convenience.

XrlArgs&
XrlArgs::add_ipvx(const char* name, const IPvX& val) throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const IPvX 
XrlArgs::get_ipvx(const char* name) const throw (XrlAtomNotFound) {
    try {
	return get(XrlAtom(name, xrlatom_ipv4)).ipv4();
    } catch (const XrlAtomNotFound&) {
	return get(XrlAtom(name, xrlatom_ipv6)).ipv6();
    }
}

void 
XrlArgs::remove_ipvx(const char* name) throw (XrlAtomNotFound) {
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
XrlArgs::add_ipvxnet(const char* name, const IPvXNet& val) 
    throw (XrlAtomFound) 
{
    return add(XrlAtom(name, val));
}

const IPvXNet 
XrlArgs::get_ipvxnet(const char* name) const throw (XrlAtomNotFound) 
{
    try {
	return get(XrlAtom(name, xrlatom_ipv4net)).ipv4net();
    } catch (const XrlAtomNotFound&) {
	return get(XrlAtom(name, xrlatom_ipv6net)).ipv6net();
    }
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
XrlArgs::add_mac(const char* name, const Mac& val) 
    throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const Mac& 
XrlArgs::get_mac(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_mac)).mac();
}

void 
XrlArgs::remove_mac(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_mac));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove string

XrlArgs&
XrlArgs::add_string(const char* name, const string& val) 
    throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const string& 
XrlArgs::get_string(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_text)).text();
}

void 
XrlArgs::remove_string(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_text));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove list

XrlArgs&
XrlArgs::add_list(const char* name, const XrlAtomList& val) 
    throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const XrlAtomList& 
XrlArgs::get_list(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_list)).list();
}

void 
XrlArgs::remove_list(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_list));
}

// ----------------------------------------------------------------------------
// XrlArgs add/get/remove binary data

XrlArgs&
XrlArgs::add_binary(const char* name, const vector<uint8_t>& val) 
    throw (XrlAtomFound) {
    return add(XrlAtom(name, val));
}

const vector<uint8_t>& 
XrlArgs::get_binary(const char* name) const throw (XrlAtomNotFound) {
    return get(XrlAtom(name, xrlatom_binary)).binary();
}

void 
XrlArgs::remove_binary(const char* name) throw (XrlAtomNotFound) {
    remove(XrlAtom(name, xrlatom_binary));
}



// ----------------------------------------------------------------------------
// Append an existing XrlArgs
XrlArgs&
XrlArgs::add(const XrlArgs& args) throw (XrlAtomFound) {
    for (const_iterator ci = args.begin(); ci != args.end(); ci++)
	add(*ci);
    return *this;
}

// ----------------------------------------------------------------------------
// Misc functions

bool
XrlArgs::matches_template(XrlArgs& t) const {
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
XrlArgs::operator==(const XrlArgs& t) const {
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
    const_iterator ai = _args.begin();
    while (index != 0 && ai != _args.end()) {
        index--;
    }
    if (ai == _args.end()) {
        throw out_of_range("XrlArgs");
    }
    return *ai;
}

const XrlAtom&
XrlArgs::operator[](const string& name) const 
    throw (XrlAtomNotFound) 
{
    for ( const_iterator ai = _args.begin() ; ai != _args.end() ; ai++ ) {
	if (ai->name() == name)
	    return *ai;
    }
    throw XrlAtomNotFound();
    return *(_args.begin()); /* NOT REACHED */
}

size_t
XrlArgs::size() const {
    return _args.size();
}

// ----------------------------------------------------------------------------
// String serialization methods

string
XrlArgs::str() const {
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

XrlArgs::XrlArgs(const char* serialized) throw (InvalidString) {
    string s(serialized);

    for (string::iterator start = s.begin(); start < s.end(); start++) {
	string::iterator end = find(start, s.end(), XrlToken::ARG_ARG_SEP[0]);
        string tok(start, end);
	try {
	    XrlAtom xa(tok.c_str());
	    add(xa); 
	} catch (const InvalidString& is) {
	    throw is;
	}
	start = end;
    }
}

