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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __LIBXIPC_XRL_ATOM_HH__
#define __LIBXIPC_XRL_ATOM_HH__

#include <string>
#include <vector>

//#include "config.h"
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
    // ... Your type's unique enumerated name here ...
    // Changing order above will break binary compatibility
    // ...Don't forget to update xrlatom_start and xrlatom_end below...

    // Bounds for enumerations
    xrlatom_start = xrlatom_int32,	// First valid enumerated value
    xrlatom_end   = xrlatom_binary	// Last valid enumerated value
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
	NoData(const char* file, int line) :
	    XorpException("XrlAtom::NoData", file, line) {}
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

    XrlAtom() : _type(xrlatom_no_type), _have_data(false) {}
    ~XrlAtom();

    // type but no data constructors
    XrlAtom(XrlAtomType t)
	: _type(t), _have_data(false) {
	set_name("");
    }

    XrlAtom(const string& name, XrlAtomType t) throw (BadName)
	: _type(t), _have_data(false) {
	set_name(name);
    }

    XrlAtom(const char* name, XrlAtomType t) throw (BadName)
	: _type(t), _have_data(false) {
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
	: _type(xrlatom_int32), _have_data(true), _i32val(value) {
	set_name("");
    }

    XrlAtom(const char* name, int32_t value) throw (BadName)
	: _type(xrlatom_int32), _have_data(true), _i32val(value) {
	set_name(name);
    }

    // bool constructors
    explicit XrlAtom(const bool& value)
	: _type(xrlatom_boolean), _have_data(true), _boolean(value) {
	set_name("");
    }

    XrlAtom(const char* name, bool value) throw (BadName)
	: _type(xrlatom_boolean), _have_data(true), _boolean(value) {
	set_name(name);
    }

    // uint32 constructors
    explicit XrlAtom(const uint32_t& value)
	: _type(xrlatom_uint32), _have_data(true), _u32val(value) {
	set_name("");
    }

    XrlAtom(const char* name, uint32_t value) throw (BadName)
	: _type(xrlatom_uint32), _have_data(true), _u32val(value) {
	set_name(name);
    }

    // ipv4 constructors
    explicit XrlAtom(const IPv4& addr)
	: _type(xrlatom_ipv4), _have_data(true),
	_ipv4(new IPv4(addr)) {
	set_name("");
    }

    XrlAtom(const char* name, const IPv4& addr) throw (BadName)
	: _type(xrlatom_ipv4), _have_data(true), _ipv4(new IPv4(addr)) {
	set_name(name);
    }

    // ipv4net constructors
    explicit XrlAtom(const IPv4Net& subnet)
	: _type(xrlatom_ipv4net), _have_data(true),
	_ipv4net(new IPv4Net(subnet)) {
	set_name("");
    }

    XrlAtom(const char* name, const IPv4Net& subnet) throw (BadName)
	: _type(xrlatom_ipv4net), _have_data(true),
	_ipv4net(new IPv4Net(subnet)) {
	set_name(name);
    }

    // ipv6 constructors
    explicit XrlAtom(const IPv6& addr)
	: _type(xrlatom_ipv6), _have_data(true),
	_ipv6(new IPv6(addr)) {
	set_name("");
    }

    XrlAtom(const char* name, const IPv6& addr) throw (BadName)
	: _type(xrlatom_ipv6), _have_data(true), _ipv6(new IPv6(addr)) {
	set_name(name);
    }

    // ipv6 net constructors
    explicit XrlAtom(const IPv6Net& subnet)
	: _type(xrlatom_ipv6net), _have_data(true),
	_ipv6net(new IPv6Net(subnet)) {
	set_name("");
    }

    XrlAtom(const char* name, const IPv6Net& subnet) throw (BadName)
	: _type(xrlatom_ipv6net), _have_data(true),
	_ipv6net(new IPv6Net(subnet)) {
	set_name(name);
    }

    // IPvX constructors - there is no underlying IPvX type
    // data is cast to IPv4 or IPv6.
    XrlAtom(const char* name, const IPvX& ipvx) throw (BadName)
	: _have_data(true)
    {
	set_name(name);
	if (ipvx.is_ipv4()) {
	    _type = xrlatom_ipv4;
	    _ipv4 = new IPv4(ipvx.get_ipv4());
	} else if (ipvx.is_ipv6()) {
	    _type = xrlatom_ipv6;
	    _ipv6 = new IPv6(ipvx.get_ipv6());
	} else {
	    abort();
	}
    }

    // IPvXNet constructors - there is no underlying IPvXNet type
    // data is cast to IPv4Net or IPv6Net.
    XrlAtom(const char* name, const IPvXNet& ipvxnet) throw (BadName)
	: _have_data(true)
    {
	set_name(name);
	if (ipvxnet.is_ipv4()) {
	    _type = xrlatom_ipv4net;
	    _ipv4net = new IPv4Net(ipvxnet.get_ipv4Net());
	} else if (ipvxnet.is_ipv6()) {
	    _type = xrlatom_ipv6net;
	    _ipv6net = new IPv6Net(ipvxnet.get_ipv6Net());
	} else {
	    abort();
	}
    }

    // mac constructors
    explicit XrlAtom(const Mac& mac)
	: _type(xrlatom_mac), _have_data(true),
	_mac(new Mac(mac)) {
	set_name("");
    }

    XrlAtom(const char* name, const Mac& mac) throw (BadName)
	: _type(xrlatom_mac), _have_data(true),
	_mac(new Mac(mac)) {
	set_name(name);
    }

    // text constructors
    explicit XrlAtom(const string& txt)
	: _type(xrlatom_text), _have_data(true),
        _text(new string(txt)) {
	set_name("");
    }

    XrlAtom(const char* name, const string& txt) throw (BadName)
	: _type(xrlatom_text), _have_data(true),
        _text(new string(txt)) {
	set_name(name);
    }

   // list constructors
    explicit XrlAtom(const XrlAtomList& l)
	: _type(xrlatom_list), _have_data(true),
	_list(new XrlAtomList(l)) {
	set_name("");
    }

    XrlAtom(const char* name, const XrlAtomList& l) throw (BadName)
	: _type(xrlatom_list), _have_data(true), _list(new XrlAtomList(l)) {
	set_name(name);
    }

    // binary
    XrlAtom(const char* name, const vector<uint8_t>& data)
	: _type(xrlatom_binary), _have_data(true),
	  _binary(new vector<uint8_t>(data)) {
	set_name(name);
    }

    XrlAtom(const char* name, const uint8_t* data, size_t data_bytes)
	: _type(xrlatom_binary), _have_data(true),
	  _binary(new vector<uint8_t>(data, data + data_bytes)) {
	set_name(name);
    }

    XrlAtom(const vector<uint8_t>& data)
	: _type(xrlatom_binary), _have_data(true),
	  _binary(new vector<uint8_t>(data)) {
	set_name("");
    }

    XrlAtom(const uint8_t* data, size_t data_bytes)
	: _type(xrlatom_binary), _have_data(true),
	  _binary(new vector<uint8_t>(data, data + data_bytes)) {
	set_name("");
    }

    // ... Your type's constructors here ...

    // Copy operations
    inline XrlAtom(const XrlAtom& x) {
	copy(x);
    }
    inline XrlAtom& operator=(const XrlAtom& x) {
	discard_dynamic(); copy(x); return *this;
    }
    void copy(const XrlAtom& x);

    // Accessor operations
    inline const string& name() const { return _atom_name; }
    void set_name(const string& n) throw (BadName) { set_name (n.c_str()); }

    string str() const;
    const string type_name() const;
    const string value() const;

    inline const bool&		has_data() const { return _have_data; }
    inline const XrlAtomType&	type() const { return _type; }

    // The following accessors may throw accessor exceptions...
    const bool&		   boolean() const throw (NoData, WrongType);
    const int32_t&	   int32() const throw (NoData, WrongType);
    const uint32_t&	   uint32() const throw (NoData, WrongType);
    const IPv4&		   ipv4() const throw (NoData, WrongType);
    const IPv4Net&	   ipv4net() const throw (NoData, WrongType);
    const IPv6&		   ipv6() const throw (NoData, WrongType);
    const IPv6Net&	   ipv6net() const throw (NoData, WrongType);
    const IPvX		   ipvx() const throw (NoData, WrongType);
    const IPvXNet	   ipvxnet() const throw (NoData, WrongType);
    const Mac&		   mac() const throw (NoData, WrongType);
    const string&	   text() const throw (NoData, WrongType);
    const XrlAtomList&	   list() const throw (NoData, WrongType);
    const vector<uint8_t>& binary() const throw (NoData, WrongType);

    // ... Your type's accessor method here ...

    // Equality tests
    bool operator==(const XrlAtom& x) const;

    // Binary packing and unpacking operations
    inline bool  packed_bytes_fixed() const;
    size_t packed_bytes() const;
    size_t pack(uint8_t* buffer, size_t bytes_available) const;

    size_t unpack(const uint8_t* buffer, size_t buffer_bytes);

    static bool valid_type(const string& s);
    static bool valid_name(const string& s);
    inline static XrlAtomType lookup_type(const string& s)
    { return resolve_type_c_str(s.c_str()); }

private:

    inline void discard_dynamic();
    inline void type_and_data_okay(const XrlAtomType& t) const
	throw (NoData, WrongType);

    void set_name(const char *n) throw (BadName);
    static XrlAtomType resolve_type_c_str(const char*);
    ssize_t data_from_c_str(const char* c_str);

    inline size_t pack_name(uint8_t* buffer) const;
    inline size_t pack_boolean(uint8_t* buffer) const;
    inline size_t pack_uint32(uint8_t* buffer) const;
    inline size_t pack_ipv4(uint8_t* buffer) const;
    inline size_t pack_ipv4net(uint8_t* buffer) const;
    inline size_t pack_ipv6(uint8_t* buffer) const;
    inline size_t pack_ipv6net(uint8_t* buffer) const;
    inline size_t pack_mac(uint8_t* buffer) const;
    inline size_t pack_text(uint8_t* buffer) const;
    inline size_t pack_list(uint8_t* buffer, size_t buffer_bytes) const;
    inline size_t pack_binary(uint8_t* buffer) const;

    inline size_t unpack_name(const uint8_t* buffer, size_t buffer_bytes)
	throw (BadName);
    inline size_t unpack_boolean(const uint8_t* buffer);
    inline size_t unpack_uint32(const uint8_t* buffer);
    inline size_t unpack_ipv4(const uint8_t* buffer);
    inline size_t unpack_ipv4net(const uint8_t* buffer);
    inline size_t unpack_ipv6(const uint8_t* buffer);
    inline size_t unpack_ipv6net(const uint8_t* buffer);
    inline size_t unpack_mac(const uint8_t* buffer, size_t buffer_bytes);
    inline size_t unpack_text(const uint8_t* buffer, size_t buffer_bytes);
    inline size_t unpack_list(const uint8_t* buffer, size_t buffer_bytes);
    inline size_t unpack_binary(const uint8_t* buffer, size_t buffer_bytes);

private:
    XrlAtomType	_type;
    bool	_have_data;
    string	_atom_name;

    union {
	bool		 _boolean;
        int32_t		 _i32val;
        uint32_t	 _u32val;
        IPv4*		 _ipv4;
        IPv4Net*	 _ipv4net;
        IPv6*		 _ipv6;
        IPv6Net*	 _ipv6net;
        Mac*		 _mac;
        string*		 _text;
	XrlAtomList*	 _list;
	vector<uint8_t>* _binary;
        // ... Your type here, if it's more than 4 bytes use a pointer ...
    } ;
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
    inline const string& atom_name() const { return _xa.name(); }
    inline const XrlAtomType& atom_type() const { return _xa.type(); }
    inline const XrlAtom& atom() const { return _xa; }
    inline const string& spell() const { return _spell; }

protected:
    const XrlAtom	_xa;
    const string	_spell;
};

#endif // __LIBXIPC_XRL_ATOM_HH__
