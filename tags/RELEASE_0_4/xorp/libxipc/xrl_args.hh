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

// $XORP: xorp/libxipc/xrl_args.hh,v 1.5 2003/03/16 08:20:31 pavlin Exp $

#ifndef __LIBXIPC_XRL_ARGS_HH__
#define __LIBXIPC_XRL_ARGS_HH__

#include <list>

#include "config.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/mac.hh"
#include "libxorp/exceptions.hh"

#include "xrl_atom.hh"

class XrlArgs {
public:
    XrlArgs() {}
    ~XrlArgs() {}

    class XrlAtomNotFound { };
    class XrlAtomFound { };

    XrlArgs& add(const XrlAtom& xa) throw (XrlAtomFound);

    const XrlAtom& get(const XrlAtom& dataless) const throw (XrlAtomNotFound);

    void remove(const XrlAtom& dataless) throw (XrlAtomNotFound);

    /* --- bool accessors --- */

    XrlArgs& add_bool(const char* name, bool val) throw (XrlAtomFound);

    const bool_t& get_bool(const char* name) const throw (XrlAtomNotFound);

    void remove_bool(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, bool v) throw (XrlAtomFound)
    {
	return add_bool(n, v);
    }

    inline void get(const char* n, bool& t) const throw (XrlAtomNotFound)
    {
	t = get_bool(n);
    }

    /* --- int32 accessors --- */

    XrlArgs& add_int32(const char* name, int32_t val) throw (XrlAtomFound);

    const int32_t& get_int32(const char* name) const throw (XrlAtomNotFound);

    void remove_int32(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, int32_t v) throw (XrlAtomFound)
    {
	return add_int32(n, v);
    }

    inline void get(const char* n, int32_t& t) const throw (XrlAtomNotFound)
    {
	t = get_int32(n);
    }

    /* --- uint32 accessors --- */

    XrlArgs& add_uint32(const char* name, uint32_t v) throw (XrlAtomFound);

    const uint32_t& get_uint32(const char* name) const throw (XrlAtomNotFound);

    void remove_uint32(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, uint32_t v) throw (XrlAtomFound)
    {
	return add_uint32(n, v);
    }

    inline void get(const char* n, uint32_t& t) const
	throw (XrlAtomNotFound) {
	t = get_uint32(n);
    }

    /* --- ipv4 accessors --- */

    XrlArgs& add_ipv4(const char* n, const IPv4& a) throw (XrlAtomFound);

    const IPv4& get_ipv4(const char* name) const throw (XrlAtomNotFound);

    void remove_ipv4(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv4& a) throw (XrlAtomFound)
    {
	return add_ipv4(n, a);
    }

    inline void get(const char* n, IPv4& a) const
	throw (XrlAtomNotFound)
    {
	a = get_ipv4(n);
    }

    /* --- ipv4net accessors --- */

    XrlArgs& add_ipv4net(const char* n, const IPv4Net& a)
	throw (XrlAtomFound);

    const IPv4Net& get_ipv4net(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipv4net(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv4Net& v) throw (XrlAtomFound)
    {
	return add_ipv4net(n, v);
    }

    inline void get(const char* n, IPv4Net& t) const
	throw (XrlAtomNotFound) {
	t = get_ipv4net(n);
    }

    /* --- ipv6 accessors --- */

    XrlArgs& add_ipv6(const char* name, const IPv6& addr)
	throw (XrlAtomFound);

    const IPv6& get_ipv6(const char* name) const throw (XrlAtomNotFound);

    void remove_ipv6(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv6& a) throw (XrlAtomFound)
    {
	return add_ipv6(n, a);
    }

    inline void get(const char* n, IPv6& a) const
	throw (XrlAtomNotFound)
    {
	a = get_ipv6(n);
    }

    /* --- ipv6net accessors --- */

    XrlArgs& add_ipv6net(const char* name, const IPv6Net& addr)
	throw (XrlAtomFound);

    const IPv6Net& get_ipv6net(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipv6net(const char* name) throw (XrlAtomNotFound);


    inline XrlArgs& add(const char* n, const IPv6Net& a) throw (XrlAtomFound)
    {
	return add_ipv6net(n, a);
    }

    inline void get(const char* n, IPv6Net& a) const
	throw (XrlAtomNotFound)
    {
	a = get_ipv6net(n);
    }

    /* --- ipvx accessors --- */

    XrlArgs& add_ipvx(const char* name, const IPvX& ipvx)
	throw (XrlAtomFound);

    const IPvX get_ipvx(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipvx(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPvX& a) throw (XrlAtomFound)
    {
	return add_ipvx(n, a);
    }

    inline void get(const char* n, IPvX& a) const
	throw (XrlAtomNotFound)
    {
	a = get_ipvx(n);
    }

    /* --- ipvxnet accessors --- */

    XrlArgs& add_ipvxnet(const char* name, const IPvXNet& ipvxnet)
	throw (XrlAtomFound);

    const IPvXNet get_ipvxnet(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipvxnet(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPvXNet& a) throw (XrlAtomFound)
    {
	return add_ipvxnet(n, a);
    }

    inline void get(const char* n, IPvXNet& a) const
	throw (XrlAtomNotFound)
    {
	a = get_ipvxnet(n);
    }

    /* --- mac accessors --- */

    XrlArgs& add_mac(const char* name, const Mac& addr)
	throw (XrlAtomFound);

    const Mac& get_mac(const char* name) const
	throw (XrlAtomNotFound);

    void remove_mac(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const Mac& a) throw (XrlAtomFound)
    {
	return add_mac(n, a);
    }

    inline void get(const char* n, Mac& a) const
	throw (XrlAtomNotFound)
    {
	a = get_mac(n);
    }

    /* --- string accessors --- */

    XrlArgs& add_string(const char* name, const string& addr)
	throw (XrlAtomFound);

    const string& get_string(const char* name) const throw (XrlAtomNotFound);

    void remove_string(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const string& a) throw (XrlAtomFound)
    {
	return add_string(n, a);
    }

    inline void get(const char* n, string& a) const
	throw (XrlAtomNotFound)
    {
	a = get_string(n);
    }

    /* --- list accessors --- */

    XrlArgs& add_list(const char* name, const XrlAtomList& addr)
	throw (XrlAtomFound);

    const XrlAtomList& get_list(const char* name)
	const throw (XrlAtomNotFound);

    void remove_list(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const XrlAtomList& a)
	throw (XrlAtomFound)
    {
	return add_list(n, a);
    }

    inline void get(const char* n, XrlAtomList& a) const
	throw (XrlAtomNotFound)
    {
	a = get_list(n);
    }

    /* --- binary data accessors --- */
    XrlArgs& add_binary(const char* name, const vector<uint8_t>& addr)
	throw (XrlAtomFound);

    const vector<uint8_t>& get_binary(const char* name) const
	throw (XrlAtomNotFound);

    void remove_binary(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const vector<uint8_t>& a)
	throw (XrlAtomFound)
    {
	return add_binary(n, a);
    }

    inline void get(const char* n, vector<uint8_t>& a) const
	throw (XrlAtomNotFound)
    {
	a = get_binary(n);
    }

    // ... Add your type's add, get, remove functions here ...

    // Append all atoms from an existing XrlArgs structure
    XrlArgs& add(const XrlArgs& args) throw (XrlAtomFound);

    // Equality testing
    bool matches_template(XrlArgs& t) const;
    bool operator==(const XrlArgs& t) const;

    // Accessor helpers
    size_t size() const;

    const XrlAtom& operator[](uint32_t index) const; // throw out_of_range
    inline const XrlAtom& item(uint32_t index) const
    { return operator[](index); }

    const XrlAtom& operator[](const string& name) const
	throw (XrlAtomNotFound);

    inline const XrlAtom& item(const string& name) const
	throw (XrlAtomNotFound)
    { return operator[](name); }

    typedef list<XrlAtom>::const_iterator const_iterator;
    const_iterator begin() const { return _args.begin(); }
    const_iterator end() const   { return _args.end(); }

    inline void clear() { _args.clear(); }
    inline bool empty() { return _args.empty(); }

    // String serialization methods
    string str() const;

    //    class InvalidString {} ;
    explicit XrlArgs(const char* str) throw (InvalidString);

    inline void swap(XrlArgs& xa) { _args.swap(xa._args); }
    
protected:
    void check_not_found(const XrlAtom &xa) throw (XrlAtomFound);

    list<XrlAtom> _args;
    typedef list<XrlAtom>::iterator iterator;
};

#endif // __LIBXIPC_XRL_ARGS_HH__
