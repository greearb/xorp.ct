// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __LIBXIPC_XRL_ATOM_HH__
#define __LIBXIPC_XRL_ATOM_HH__

#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/mac.hh"




#include "xrl_atom_list.hh"
#include "fp64.h"


enum XrlAtomType {
    xrlatom_no_type = 0,
    xrlatom_int32,
    xrlatom_uint32,
    xrlatom_ipv4,
    xrlatom_ipv4net,
    xrlatom_ipv6,
    xrlatom_ipv6net,
    xrlatom_mac,
    xrlatom_text,
    xrlatom_list,
    xrlatom_boolean,
    xrlatom_binary,
    xrlatom_int64,
    xrlatom_uint64,
    xrlatom_fp64,
    // ... Your type's unique enumerated name here ...
    // Changing order above will break binary compatibility
    // ...Don't forget to update xrlatom_start and xrlatom_end below...

    // Bounds for enumerations
    xrlatom_start = xrlatom_int32,	// First valid enumerated value
    xrlatom_end   = xrlatom_fp64	// Last valid enumerated value
};

inline XrlAtomType& operator++(XrlAtomType& t)
{
    return t = (xrlatom_end == t) ? xrlatom_no_type : XrlAtomType(t + 1);
}

/**
 * @return name of atom corresponding to type.
 */
const char* xrlatom_type_name(const XrlAtomType&);

class XrlAtom {
public:
    // Exceptions
    struct NoData : public XorpException {
	NoData(const char* file, int line, const string& name) :
	    XorpException("XrlAtom::NoData", file, line),
	_name(name) {}

	const string why() const {
	    return c_format("Atom name %s has no data", _name.c_str());
	}

    private:
	string _name;
    };

    struct WrongType : public XorpException {
	WrongType(const char* file, int line,
		  const XrlAtomType& actual, const XrlAtomType& expected) :
	    XorpException("XrlAtom::WrongType", file, line),
	    _actual(actual), _expected(expected) {}

	const string why() const {
	    return c_format("Atom type %s (%d) expected %s (%d)",
			    xrlatom_type_name(_actual), _actual,
			    xrlatom_type_name(_expected), _expected);
	}
    private:
	XrlAtomType _actual;
	XrlAtomType _expected;
    };

    struct BadName : public XorpException {
	BadName(const char* file, int line, const char* name) :
	    XorpException("XrlAtom::BadName", file, line), _name(name) {}
	const string why() const {
	    return c_format("\"%s\" is not a valid name", _name.c_str());
	}
    private:
	string _name;
    };

    XrlAtom() : _type(xrlatom_no_type), _have_data(false), _own(true), _has_fake_args(false) {}
    ~XrlAtom();

    // type but no data constructors
    XrlAtom(XrlAtomType t)
	: _type(t), _have_data(false), _own(true), _has_fake_args(false) {}

    XrlAtom(const string& name, XrlAtomType t) throw (BadName)
	: _type(t), _have_data(false), _own(true), _has_fake_args(false) {
	set_name(name);
    }

    XrlAtom(const char* name, XrlAtomType t) throw (BadName)
	: _type(t), _have_data(false), _own(true), _has_fake_args(false) {
	set_name(name);
    }

    XrlAtom(const string& name, XrlAtomType t, const string& serialized_data)
	throw (InvalidString);

    XrlAtom(const char* name, XrlAtomType t, const string& serialized_data)
	throw (InvalidString);

    /**
     * Construct an XrlAtom from it's serialized character representation.
     *
     * Be careful not confuse this with the unnamed string atom constructor
     * XrlAtom(const string&).
     *
     */
    explicit XrlAtom(const char*) throw (InvalidString, BadName);

    // int32 constructors
    explicit XrlAtom(const int32_t& value)
	: _type(xrlatom_int32), _have_data(true), _own(true), _has_fake_args(false), _i32val(value) {}

    XrlAtom(const char* name, int32_t value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_int32), _have_data(true), _own(true), _has_fake_args(fake_args),_i32val(value) {
	set_name(name);
    }

    // bool constructors
    explicit XrlAtom(const bool& value)
	: _type(xrlatom_boolean), _have_data(true),
	  _own(true), _has_fake_args(false), _boolean(value) {}

    XrlAtom(const char* name, bool value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_boolean), _have_data(true),
	  _own(true), _has_fake_args(fake_args), _boolean(value) {
	set_name(name);
    }

    // uint32 constructors
    explicit XrlAtom(const uint32_t& value)
	: _type(xrlatom_uint32), _have_data(true), _own(true), _has_fake_args(false), _u32val(value) {}

    XrlAtom(const char* name, uint32_t value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_uint32), _have_data(true), _own(true), _has_fake_args(fake_args), _u32val(value) {
	set_name(name);
    }

    // ipv4 constructors
    explicit XrlAtom(const IPv4& addr)
	: _type(xrlatom_ipv4), _have_data(true), _own(true), _has_fake_args(false),
	  _ipv4(addr) {}

    XrlAtom(const char* name, const IPv4& addr, bool fake_args = false) throw (BadName)
	: _type(xrlatom_ipv4), _have_data(true), _own(true), _has_fake_args(fake_args),
	  _ipv4(addr) {
	set_name(name);
    }

    // ipv4net constructors
    explicit XrlAtom(const IPv4Net& subnet)
	    : _type(xrlatom_ipv4net), _have_data(true), _own(true), _has_fake_args(false),
	      _ipv4net(subnet) {}

    XrlAtom(const char* name, const IPv4Net& subnet, bool fake_args = false) throw (BadName)
	    : _type(xrlatom_ipv4net), _have_data(true), _own(true), _has_fake_args(fake_args),
	      _ipv4net(subnet) {
	set_name(name);
    }

    // ipv6 constructors
    explicit XrlAtom(const IPv6& addr)
	: _type(xrlatom_ipv6), _have_data(true), _own(true), _has_fake_args(false),
	_ipv6(new IPv6(addr)) {}

    XrlAtom(const char* name, const IPv6& addr) throw (BadName)
	: _type(xrlatom_ipv6), _have_data(true), _own(true), _has_fake_args(false),
	  _ipv6(new IPv6(addr)) {
	set_name(name);
    }

    // ipv6 net constructors
    explicit XrlAtom(const IPv6Net& subnet)
	: _type(xrlatom_ipv6net), _have_data(true), _own(true), _has_fake_args(false),
	_ipv6net(new IPv6Net(subnet)) {}

    XrlAtom(const char* name, const IPv6Net& subnet, bool fake_args = false) throw (BadName)
	: _type(xrlatom_ipv6net), _have_data(true), _own(true), _has_fake_args(fake_args),
	_ipv6net(new IPv6Net(subnet)) {
	set_name(name);
    }

    // IPvX constructors - there is no underlying IPvX type
    // data is cast to IPv4 or IPv6.
    XrlAtom(const char* name, const IPvX& ipvx, bool fake_args = false) throw (BadName)
	: _have_data(true), _own(true), _has_fake_args(fake_args)
    {
	set_name(name);
	if (ipvx.is_ipv4()) {
	    _type = xrlatom_ipv4;
	    _ipv4 = ipvx.get_ipv4();
	} else if (ipvx.is_ipv6()) {
	    _type = xrlatom_ipv6;
	    _ipv6 = new IPv6(ipvx.get_ipv6());
	} else {
	    abort();
	}
    }

    // IPvXNet constructors - there is no underlying IPvXNet type
    // data is cast to IPv4Net or IPv6Net.
    XrlAtom(const char* name, const IPvXNet& ipvxnet, bool fake_args = false) throw (BadName)
	: _have_data(true), _own(true), _has_fake_args(fake_args)
    {
	set_name(name);
	if (ipvxnet.is_ipv4()) {
	    _type = xrlatom_ipv4net;
	    _ipv4net = ipvxnet.get_ipv4net();
	} else if (ipvxnet.is_ipv6()) {
	    _type = xrlatom_ipv6net;
	    _ipv6net = new IPv6Net(ipvxnet.get_ipv6net());
	} else {
	    abort();
	}
    }

    // mac constructors
    explicit XrlAtom(const Mac& mac)
	: _type(xrlatom_mac), _have_data(true), _own(true), _has_fake_args(false),
	_mac(new Mac(mac)) {}

    XrlAtom(const char* name, const Mac& mac, bool fake_args = false) throw (BadName)
	: _type(xrlatom_mac), _have_data(true), _own(true), _has_fake_args(fake_args),
	_mac(new Mac(mac)) {
	set_name(name);
    }

    // text constructors
    explicit XrlAtom(const string& txt)
	: _type(xrlatom_text), _have_data(true), _own(true), _has_fake_args(false),
        _text(new string(txt)) {}

    XrlAtom(const char* name, const string& txt, bool fake_args = false) throw (BadName)
	: _type(xrlatom_text), _have_data(true), _own(true), _has_fake_args(fake_args),
        _text(new string(txt)) {
	set_name(name);
    }

   // list constructors
    explicit XrlAtom(const XrlAtomList& l)
	: _type(xrlatom_list), _have_data(true), _own(true), _has_fake_args(false),
	_list(new XrlAtomList(l)) {}

    XrlAtom(const char* name, const XrlAtomList& l, bool fake_args = false) throw (BadName)
	: _type(xrlatom_list), _have_data(true), _own(true), _has_fake_args(fake_args),
	_list(new XrlAtomList(l)) {
	set_name(name);
    }

    // binary
    XrlAtom(const char* name, const vector<uint8_t>& data)
	: _type(xrlatom_binary), _have_data(true), _own(true), _has_fake_args(false),
	  _binary(new vector<uint8_t>(data)) {
	set_name(name);
    }

    XrlAtom(const char* name, const uint8_t* data, size_t data_bytes, bool fake_args = false)
	: _type(xrlatom_binary), _have_data(true), _own(true), _has_fake_args(fake_args),
	  _binary(new vector<uint8_t>(data, data + data_bytes)) {
	set_name(name);
    }

    XrlAtom(const vector<uint8_t>& data)
	: _type(xrlatom_binary), _have_data(true), _own(true), _has_fake_args(false),
	  _binary(new vector<uint8_t>(data)) {}

    XrlAtom(const uint8_t* data, size_t data_bytes)
	: _type(xrlatom_binary), _have_data(true), _own(true), _has_fake_args(false),
	  _binary(new vector<uint8_t>(data, data + data_bytes)) {}

    // int64 constructors
    explicit XrlAtom(const int64_t& value)
	: _type(xrlatom_int64), _have_data(true), _own(true), _has_fake_args(false), _i64val(value) {}

    XrlAtom(const char* name, int64_t value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_int64), _have_data(true), _own(true), _has_fake_args(fake_args), _i64val(value) {
	set_name(name);
    }

    // uint64 constructors
    explicit XrlAtom(const uint64_t& value)
	: _type(xrlatom_uint64), _have_data(true), _own(true), _has_fake_args(false), _u64val(value) {}

    XrlAtom(const char* name, uint64_t value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_uint64), _have_data(true), _own(true), _has_fake_args(fake_args), _u64val(value) {
	set_name(name);
    }


    // fp64 constructors
    explicit XrlAtom(const fp64_t& value)
	: _type(xrlatom_fp64), _have_data(true), _own(true), _has_fake_args(false), _fp64val(value) {}

    XrlAtom(const char* name, fp64_t value, bool fake_args = false) throw (BadName)
	: _type(xrlatom_fp64), _have_data(true), _own(true), _has_fake_args(fake_args), _fp64val(value) {
	set_name(name);
    }


    // ... Your type's constructors here ...

    // Copy operations
    XrlAtom(const XrlAtom& x)
	: _type(xrlatom_no_type), _have_data(false), _own(true), _has_fake_args(false) {
	copy(x);
    }
    XrlAtom& operator=(const XrlAtom& x) {
	discard_dynamic(); copy(x); return *this;
    }
    void copy(const XrlAtom& x);

    // Accessor operations
    const string& name() const { return _atom_name; }
    void set_name(const string& n) throw (BadName) { set_name (n.c_str()); }

    string str() const;
    const char* type_name() const;
    const string value() const;

    const bool&		has_data() const { return _have_data; }
    const bool&		has_fake_args() const { return _has_fake_args; }
    const XrlAtomType&	type() const { return _type; }

    void using_real_args() { _has_fake_args = false; }

    // The following accessors may throw accessor exceptions...
    const bool&		   boolean() const throw (NoData, WrongType);
    const int32_t&	   int32() const throw (NoData, WrongType);
    const uint32_t&	   uint32() const throw (NoData, WrongType);
    const IPv4&		   ipv4() const throw (NoData, WrongType);
    const IPv4Net&         ipv4net() const throw (NoData, WrongType);
    const IPv6&		   ipv6() const throw (NoData, WrongType);
    const IPv6Net&	   ipv6net() const throw (NoData, WrongType);
    const IPvX		   ipvx() const throw (NoData, WrongType);
    const IPvXNet	   ipvxnet() const throw (NoData, WrongType);
    const Mac&		   mac() const throw (NoData, WrongType);
    const string&	   text() const throw (NoData, WrongType);
    const XrlAtomList&	   list() const throw (NoData, WrongType);
    const vector<uint8_t>& binary() const throw (NoData, WrongType);
    const int64_t&	   int64() const throw (NoData, WrongType);
    const uint64_t&	   uint64() const throw (NoData, WrongType);
    const fp64_t&	   fp64() const throw (NoData, WrongType);

    // ... Your type's accessor method here ...

    // set methods
#define SET(type, var, p)		    \
    void set(const type& arg)		    \
    {					    \
	abandon_data();			    \
					    \
	type& a = const_cast<type&>(arg);   \
	var = p a;			    \
    }

    SET(int32_t, _i32val, )
    SET(uint32_t, _u32val, )
    SET(int64_t, _i64val, )
    SET(uint64_t, _u64val, )
    SET(bool, _boolean, )
    SET(IPv6, _ipv6, &)
    SET(IPv6Net, _ipv6net, &)
    SET(Mac, _mac, &)
    SET(string, _text, &)
    SET(XrlAtomList, _list, &)
    SET(vector<uint8_t>, _binary, &);
    SET(fp64_t, _fp64val, )
#undef SET

    void set(const IPv4& v) {
	abandon_data();
	_ipv4 = v;
    }

    void set(const IPv4Net& v) {
	abandon_data();
	_ipv4net = v;
    }

    // Equality tests
    bool operator==(const XrlAtom& x) const;

    // Binary packing and unpacking operations
    bool packed_bytes_fixed() const;
    size_t packed_bytes() const;
    size_t pack(uint8_t* buffer, size_t bytes_available) const;

    size_t unpack(const uint8_t* buffer, size_t buffer_bytes);

    static bool valid_type(const string& s);
    static bool valid_name(const string& s);
    static XrlAtomType lookup_type(const string& s) {
	return resolve_type_c_str(s.c_str());
    }

    static size_t peek_text(const char*& t, uint32_t& tl,
			    const uint8_t* buf, size_t len);

private:

    void discard_dynamic();
    void abandon_data();
    void type_and_data_okay(const XrlAtomType& t) const
	throw (NoData, WrongType);

    void set_name(const char *n) throw (BadName);
    static XrlAtomType resolve_type_c_str(const char*);
    ssize_t data_from_c_str(const char* c_str);

    size_t pack_name(uint8_t* buffer) const;
    size_t pack_boolean(uint8_t* buffer) const;
    size_t pack_uint32(uint8_t* buffer) const;
    size_t pack_ipv4(uint8_t* buffer) const;
    size_t pack_ipv4net(uint8_t* buffer) const;
    size_t pack_ipv6(uint8_t* buffer) const;
    size_t pack_ipv6net(uint8_t* buffer) const;
    size_t pack_mac(uint8_t* buffer) const;
    size_t pack_text(uint8_t* buffer) const;
    size_t pack_list(uint8_t* buffer, size_t buffer_bytes) const;
    size_t pack_binary(uint8_t* buffer) const;
    size_t pack_uint64(uint8_t* buffer) const;
    size_t pack_fp64(uint8_t* buffer) const;

    size_t unpack_name(const uint8_t* buffer, size_t buffer_bytes)
	throw (BadName);
    size_t unpack_boolean(const uint8_t* buffer);
    size_t unpack_uint32(const uint8_t* buffer);
    size_t unpack_ipv4(const uint8_t* buffer);
    size_t unpack_ipv4net(const uint8_t* buffer);
    size_t unpack_ipv6(const uint8_t* buffer);
    size_t unpack_ipv6net(const uint8_t* buffer);
    size_t unpack_mac(const uint8_t* buffer, size_t buffer_bytes);
    size_t unpack_text(const uint8_t* buffer, size_t buffer_bytes);
    size_t unpack_list(const uint8_t* buffer, size_t buffer_bytes);
    size_t unpack_binary(const uint8_t* buffer, size_t buffer_bytes);
    size_t unpack_uint64(const uint8_t* buffer);
    size_t unpack_fp64(const uint8_t* buffer);

private:
    XrlAtomType	_type;
    bool	_have_data;
    string	_atom_name;
    bool	_own;
    bool	_has_fake_args;

    union {
	bool		 _boolean;
        int32_t		 _i32val;
        uint32_t	 _u32val;
        IPv6*		 _ipv6;
        IPv6Net*	 _ipv6net;
        Mac*		 _mac;
        string*		 _text;
	XrlAtomList*	 _list;
	vector<uint8_t>* _binary;
        int64_t		 _i64val;
        uint64_t	 _u64val;
        fp64_t	         _fp64val;

        // ... Your type here, if it's more than sizeof(uintptr_t) bytes,
	// use a pointer ...
    } ;

    // Can't put this in a union, evidently.
    IPv4	         _ipv4;
    IPv4Net              _ipv4net;
};

// XrlAtomSpell is a placeholder class for Xrl mapping
// It used to hold the name:type of an atom whose value we
// wish to refer to.
class XrlAtomSpell {
public:
    XrlAtomSpell(const XrlAtom& xa, const string& spell) :
	_xa(xa), _spell(spell) {}
    XrlAtomSpell(const string&	    name,
		 const XrlAtomType& type,
		 const string&	    m)
	: _xa(XrlAtom(name, type)), _spell(m) {}
#ifdef XORP_USE_USTL
    XrlAtomSpell() { }
#endif
    const string& atom_name() const { return _xa.name(); }
    const XrlAtomType& atom_type() const { return _xa.type(); }
    const XrlAtom& atom() const { return _xa; }
    const string& spell() const { return _spell; }

protected:
    XrlAtom	_xa;
    string	_spell;
};

#endif // __LIBXIPC_XRL_ATOM_HH__
