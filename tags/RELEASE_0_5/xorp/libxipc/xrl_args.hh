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

// $XORP: xorp/libxipc/xrl_args.hh,v 1.7 2003/10/18 05:22:45 hodson Exp $

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
    typedef list<XrlAtom>::const_iterator const_iterator;
    typedef list<XrlAtom>::iterator iterator;

    class XrlAtomNotFound { };
    class XrlAtomFound { };

public:
    XrlArgs() {}
    explicit XrlArgs(const char* str) throw (InvalidString);

    ~XrlArgs() {}

    /* --- XrlAtom accessors --- */
    XrlArgs& add(const XrlAtom& xa) throw (XrlAtomFound);

    const XrlAtom& get(const XrlAtom& dataless) const throw (XrlAtomNotFound);

    void remove(const XrlAtom& dataless) throw (XrlAtomNotFound);

    /* --- bool accessors --- */

    XrlArgs& add_bool(const char* name, bool val) throw (XrlAtomFound);

    const bool_t& get_bool(const char* name) const throw (XrlAtomNotFound);

    void remove_bool(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, bool v) throw (XrlAtomFound);

    inline void get(const char* n, bool& t) const throw (XrlAtomNotFound);

    /* --- int32 accessors --- */

    XrlArgs& add_int32(const char* name, int32_t val) throw (XrlAtomFound);

    const int32_t& get_int32(const char* name) const throw (XrlAtomNotFound);

    void remove_int32(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, int32_t v) throw (XrlAtomFound);

    inline void get(const char* n, int32_t& t) const throw (XrlAtomNotFound);

    /* --- uint32 accessors --- */

    XrlArgs& add_uint32(const char* name, uint32_t v) throw (XrlAtomFound);

    const uint32_t& get_uint32(const char* name) const throw (XrlAtomNotFound);

    void remove_uint32(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, uint32_t v) throw (XrlAtomFound);

    inline void get(const char* n, uint32_t& t) const throw (XrlAtomNotFound);

    /* --- ipv4 accessors --- */

    XrlArgs& add_ipv4(const char* n, const IPv4& a) throw (XrlAtomFound);

    const IPv4& get_ipv4(const char* name) const throw (XrlAtomNotFound);

    void remove_ipv4(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv4& a) throw (XrlAtomFound);

    inline void get(const char* n, IPv4& a) const throw (XrlAtomNotFound);

    /* --- ipv4net accessors --- */

    XrlArgs& add_ipv4net(const char* n, const IPv4Net& a) throw (XrlAtomFound);

    const IPv4Net& get_ipv4net(const char* name) const throw (XrlAtomNotFound);

    void remove_ipv4net(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv4Net& v) throw (XrlAtomFound);

    inline void get(const char* n, IPv4Net& t) const throw (XrlAtomNotFound);

    /* --- ipv6 accessors --- */

    XrlArgs& add_ipv6(const char* name, const IPv6& addr)
	throw (XrlAtomFound);

    const IPv6& get_ipv6(const char* name) const throw (XrlAtomNotFound);

    void remove_ipv6(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv6& a) throw (XrlAtomFound);

    inline void get(const char* n, IPv6& a) const throw (XrlAtomNotFound);

    /* --- ipv6net accessors --- */

    XrlArgs& add_ipv6net(const char* name, const IPv6Net& addr)
	throw (XrlAtomFound);

    const IPv6Net& get_ipv6net(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipv6net(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPv6Net& a) throw (XrlAtomFound);

    inline void get(const char* n, IPv6Net& a) const throw (XrlAtomNotFound);

    /* --- ipvx accessors --- */

    XrlArgs& add_ipvx(const char* name, const IPvX& ipvx)
	throw (XrlAtomFound);

    const IPvX get_ipvx(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipvx(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPvX& a) throw (XrlAtomFound);

    inline void get(const char* n, IPvX& a) const throw (XrlAtomNotFound);

    /* --- ipvxnet accessors --- */

    XrlArgs& add_ipvxnet(const char* name, const IPvXNet& ipvxnet)
	throw (XrlAtomFound);

    const IPvXNet get_ipvxnet(const char* name) const
	throw (XrlAtomNotFound);

    void remove_ipvxnet(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const IPvXNet& a) throw (XrlAtomFound);

    inline void get(const char* n, IPvXNet& a) const throw (XrlAtomNotFound);

    /* --- mac accessors --- */

    XrlArgs& add_mac(const char* name, const Mac& addr)
	throw (XrlAtomFound);

    const Mac& get_mac(const char* name) const
	throw (XrlAtomNotFound);

    void remove_mac(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const Mac& a) throw (XrlAtomFound);

    inline void get(const char* n, Mac& a) const throw (XrlAtomNotFound);

    /* --- string accessors --- */

    XrlArgs& add_string(const char* name, const string& addr)
	throw (XrlAtomFound);

    const string& get_string(const char* name) const throw (XrlAtomNotFound);

    void remove_string(const char* name) throw (XrlAtomNotFound);

    inline XrlArgs& add(const char* n, const string& a) throw (XrlAtomFound);

    inline void get(const char* n, string& a) const throw (XrlAtomNotFound);

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
	throw (XrlAtomFound);

    inline void get(const char* n, vector<uint8_t>& a) const
	throw (XrlAtomNotFound);

    // ... Add your type's add, get, remove functions here ...

    // Append all atoms from an existing XrlArgs structure
    XrlArgs& add(const XrlArgs& args) throw (XrlAtomFound);

    // Equality testing
    bool matches_template(XrlArgs& t) const;
    bool operator==(const XrlArgs& t) const;

    // Accessor helpers
    size_t size() const;

    const XrlAtom& operator[](uint32_t index) const; // throw out_of_range
    inline const XrlAtom& item(uint32_t index) const;

    const XrlAtom& operator[](const string& name) const
	throw (XrlAtomNotFound);

    inline const XrlAtom& item(const string& name) const
	throw (XrlAtomNotFound);

    inline const_iterator begin() const		{ return _args.begin(); }
    inline const_iterator end() const		{ return _args.end(); }

    inline void clear()				{ _args.clear(); }
    inline bool empty()				{ return _args.empty(); }
    inline void swap(XrlArgs& xa)		{ _args.swap(xa._args); }

    /**
     * Get number of bytes needed to pack atoms contained within
     * instance.
     */
    size_t packed_bytes() const;

    /**
     * Pack contained atoms into a byte array.  The size of the byte
     * array should be larger or equal to the value returned by
     * packed_bytes().
     *
     * @param buffer buffer to receive data.
     * @param buffer_bytes size of buffer.
     * @return size of packed data on success, 0 on failure.
     */
    size_t pack(uint8_t* buffer, size_t buffer_bytes) const;

    /**
     * Unpack atoms from byte array into instance.  The atoms are
     * appended to the list of atoms associated with instance.
     *
     * @param buffer to read data from.
     * @param buffer_bytes size of buffer.  The size should exactly match
     *        number of bytes of packed atoms, ie packed_bytes() value
     *        used when packing.
     * @return number of bytes turned into atoms on success, 0 on failure.
     */
    size_t unpack(const uint8_t* buffer, size_t buffer_bytes);

    // String serialization methods
    string str() const;

protected:
    void check_not_found(const XrlAtom &xa) throw (XrlAtomFound);

    list<XrlAtom> _args;
};


// ----------------------------------------------------------------------------
// Inline methods

inline XrlArgs&
XrlArgs::add(const char* n, bool v) throw (XrlAtomFound)
{
    return add_bool(n, v);
}

inline void
XrlArgs::get(const char* n, bool& t) const throw (XrlAtomNotFound)
{
    t = get_bool(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, int32_t v) throw (XrlAtomFound)
{
    return add_int32(n, v);
}

inline void
XrlArgs::get(const char* n, int32_t& t) const throw (XrlAtomNotFound)
{
    t = get_int32(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, uint32_t v) throw (XrlAtomFound)
{
    return add_uint32(n, v);
}

inline void
XrlArgs::get(const char* n, uint32_t& t) const throw (XrlAtomNotFound)
{
    t = get_uint32(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPv4& a) throw (XrlAtomFound)
{
    return add_ipv4(n, a);
}

inline void
XrlArgs::get(const char* n, IPv4& a) const throw (XrlAtomNotFound)
{
    a = get_ipv4(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPv4Net& v) throw (XrlAtomFound)
{
    return add_ipv4net(n, v);
}

inline void
XrlArgs::get(const char* n, IPv4Net& t) const throw (XrlAtomNotFound)
{
    t = get_ipv4net(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPv6& a) throw (XrlAtomFound)
{
    return add_ipv6(n, a);
}

inline void
XrlArgs::get(const char* n, IPv6& a) const throw (XrlAtomNotFound)
{
    a = get_ipv6(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPv6Net& a) throw (XrlAtomFound)
{
    return add_ipv6net(n, a);
}

inline void
XrlArgs::get(const char* n, IPv6Net& a) const throw (XrlAtomNotFound)
{
    a = get_ipv6net(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPvX& a) throw (XrlAtomFound)
{
    return add_ipvx(n, a);
}

inline void
XrlArgs::get(const char* n, IPvX& a) const throw (XrlAtomNotFound)
{
    a = get_ipvx(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const IPvXNet& a) throw (XrlAtomFound)
{
    return add_ipvxnet(n, a);
}

inline void
XrlArgs::get(const char* n, IPvXNet& a) const throw (XrlAtomNotFound)
{
    a = get_ipvxnet(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const Mac& a) throw (XrlAtomFound)
{
    return add_mac(n, a);
}

inline void
XrlArgs::get(const char* n, Mac& a) const throw (XrlAtomNotFound)
{
    a = get_mac(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const string& a) throw (XrlAtomFound)
{
    return add_string(n, a);
}

inline void
XrlArgs::get(const char* n, string& a) const throw (XrlAtomNotFound)
{
    a = get_string(n);
}

inline XrlArgs&
XrlArgs::add(const char* n, const vector<uint8_t>& a) throw (XrlAtomFound)
{
    return add_binary(n, a);
}

inline void
XrlArgs::get(const char* n, vector<uint8_t>& a) const throw (XrlAtomNotFound)
{
    a = get_binary(n);
}

inline const XrlAtom&
XrlArgs::item(uint32_t index) const
{
    return operator[](index);
}

inline const XrlAtom&
XrlArgs::item(const string& name) const throw (XrlAtomNotFound)
{
    return operator[](name);
}


#endif // __LIBXIPC_XRL_ARGS_HH__
