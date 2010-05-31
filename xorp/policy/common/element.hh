// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/element.hh,v 1.17 2008/10/02 21:58:06 bms Exp $

#ifndef __POLICY_COMMON_ELEMENT_HH__
#define __POLICY_COMMON_ELEMENT_HH__

#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "element_base.hh"
#include "policy_exception.hh"
#include "policy_utils.hh"
#include "policy/policy_module.h"
#include "operator_base.hh"

enum {
    HASH_ELEM_INT32 = 1,
    HASH_ELEM_U32,
    HASH_ELEM_COM32,
    HASH_ELEM_STR,
    HASH_ELEM_BOOL,		// 5
    HASH_ELEM_IPV4,
    HASH_ELEM_IPV6,
    HASH_ELEM_IPV4RANGE,
    HASH_ELEM_IPV6RANGE,
    HASH_ELEM_IPV4NET,		// 10
    HASH_ELEM_IPV6NET,
    HASH_ELEM_U32RANGE,
    HASH_ELEM_SET_U32,
    HASH_ELEM_SET_COM32,
    HASH_ELEM_SET_IPV4NET,	// 15
    HASH_ELEM_SET_IPV6NET,
    HASH_ELEM_SET_STR,
    HASH_ELEM_NULL,
    HASH_ELEM_FILTER,
    HASH_ELEM_ASPATH,		// 20
    HASH_ELEM_IPV4NEXTHOP,
    HASH_ELEM_IPV6NEXTHOP,

    HASH_ELEM_MAX = 32 // must be last
};

/**
 * @short 32bit signed integer.
 */
class ElemInt32 : public Element {
public:
    /**
     * The identifier [type] of the element.
     */
    static const char* id;
    static Hash _hash;

    ElemInt32() : Element(_hash) {}

    /**
     * Construct via c-style string.
     *
     * This is necessary in order to create elements via the ElementFactory.
     * If c_str is null, then the element is assigned a default value. Null
     * c_str is used by the semantic checker, to obtain "dummy" elements for
     * validity checks.
     *
     * @param c_str initialize via string, or assign default value if null.
     */
    ElemInt32(const char* c_str) : Element(_hash)
    {
	if (c_str)
	    _val = strtol(c_str,NULL,10);
	else
	    _val = 0;
    }

    ElemInt32(const int32_t val) : Element(_hash), _val(val) {}

    /**
     * @return string representation of integer
     */
    string str() const {
	return policy_utils::to_str(_val);
    }

    /**
     * @return value of the element.
     */
    int32_t val() const { return _val; }

    const char* type() const { return id; }

private:
    int32_t _val;
};

/**
 * @short 32bit unsigned integer.
 */
class ElemU32 : public Element {
public:
    static const char* id;
    static Hash _hash;

    ElemU32() : Element(_hash) {}

    ElemU32(const char* c_str) : Element(_hash)
    {
	if (c_str)
	    _val = strtoul(c_str,NULL,10); 
	else
	    _val = 0;
    }

    ElemU32(const uint32_t val) : Element(_hash), _val(val) {}

    string str() const
    {
	return policy_utils::to_str(_val);
    }

    uint32_t val() const { return _val; }
    const char* type() const { return id; }

    bool operator==(const ElemU32& rhs) const { return _val == rhs._val; }
    bool operator<(const ElemU32& rhs) const { return _val < rhs._val; }

private:
    uint32_t _val;
};

/**
 * @short 32bit unsigned integer with BGP communities friendly syntax.
 *    "X"  -> (uint32_t) X
 *   ":X"  -> (uint16_t) X
 *  "X:"   -> ((uint16_t) X) << 16
 *  "X:Y"  -> (((uint16_t) X) << 16) + (uint16_t) Y
 */
class ElemCom32 : public Element {
public:
    static const char* id;
    static Hash _hash;

    ElemCom32() : Element(_hash) {}
    ElemCom32(const char*);		// in element.cc
    ElemCom32(const uint32_t val) : Element(_hash), _val(val) {}

    string str() const;			// in element.cc
    uint32_t val() const { return _val; }
    const char* type() const { return id; }

    bool operator==(const ElemCom32& rhs) const { return _val == rhs._val; }
    bool operator<(const ElemCom32& rhs) const { return _val < rhs._val; }


private:
    uint32_t _val;
};

/**
 * @short string element.
 */
class ElemStr : public Element {
public:
    static const char* id;
    static Hash _hash;

    ElemStr() : Element(_hash) {}

    ElemStr(const char* val) : Element(_hash)
    {
	if (val)
	    _val = val;
	else
	    _val = "";
    }

    ElemStr(const string& str) : Element(_hash), _val(str) {}

    string str() const { return _val; }
    string val() const { return _val; }
    const char* type() const { return id; }

    bool operator==(const ElemStr& rhs) const { return _val == rhs._val; }
    bool operator<(const ElemStr& rhs) const { return _val < rhs._val; }

private:
    string _val;
};

/**
 * @short boolean element.
 */
class ElemBool : public Element {
public:
    static const char* id;
    static Hash _hash;

    ElemBool() : Element(_hash) {}

    ElemBool(const char* c_str) : Element(_hash)
    {
	if (c_str && (strcmp(c_str,"true") == 0) )
	    _val = true;
	else
	    _val = false;
    }

    ElemBool(const bool val) : Element(_hash), _val(val) {}

    string str() const
    {
	if (_val)
	    return "true";
	else
	    return "false";
    }	    

    bool val() const { return _val; }
    const char* type() const { return id; }

    bool operator==(const ElemBool& rhs) const { return _val == rhs._val; }

private:
    bool _val;
};


/**
 * @short Generic Element wrapper for existing classes.
 *
 * Classes being wrapped need to provide a str() method and must have a c-style
 * string constructor.
 * They also must support various relational operators such as ==.
 */
template<class T>
class ElemAny : public Element {
public:
    
    /**
     * @short exception thrown if c-stype string initialization fails.
     */
    class ElemInitError : public PolicyException {
    public:
	ElemInitError(const char* file, size_t line, const string& init_why = "")   
	    : PolicyException("ElemInitError", file, line, init_why) {}  
    };


    static const char* id;
    static Hash _hash;

    ElemAny() : Element(_hash), _val() {}
    ElemAny(const T& val) : Element(_hash), _val(val) {}

    /**
     * If the c-style constructor of the wrapped class throws and exception,
     * it is caught and an ElemInitError exception is thrown. The original
     * exception is lost.
     */
    ElemAny(const char* c_str) : Element(_hash), _val()
    {
	if (c_str) {
	    try {
		_val = T(c_str);
	    } catch (...) {
		string err = "Unable to initialize element of type ";
		err += id;
		err += " with ";
		err += c_str;

		xorp_throw(ElemInitError, err);

	    }
	    
	}
	// else leave it to the default value
    }

    /**
     * Invokes the == operator on the actual classes being wrapped.
     *
     * @return whether the two values are equal.
     * @param rhs element to compare with.
     */
    bool operator==(const ElemAny<T>& rhs) const {
	return _val == rhs._val;
    }

    /**
     * Invoke the < operator in the wrapped class.
     *
     * @return whether this is less than argument.
     * @param rhs element to compare with.
     */
    bool operator<(const ElemAny<T>& rhs) const {
	return _val < rhs._val;
    }
    
    /**
     * Invokes the str() method of the actual class being wrapped.
     *
     * @return string representation of element.
     */
    string str() const { return _val.str(); }

    /**
     * @return the actual object of the class being wrapped.
     */
    const T& val() const { return _val; }
    
    const char* type() const { return id; }

private:
    T _val;
};

template<class T>
class ElemRefAny : public Element {
public:
    /**
     * @short exception thrown if c-stype string initialization fails.
     */
    class ElemInitError : public PolicyException {
    public:
	ElemInitError(const char* file, size_t line, const string& init_why = "")   
	    : PolicyException("ElemInitError", file, line, init_why) {}
    };

    
    static const char* id;
    static Hash _hash;

    ElemRefAny() : Element(_hash), _val(new T()), _free(true) {}
    ElemRefAny(const T& val) : Element(_hash), _val(&val), _free(false) {}
    ElemRefAny(const T& val, bool f) : Element(_hash), _val(&val), _free(f) {}

    ~ElemRefAny() { if (_free) delete _val; }

    /**
     * If the c-style constructor of the wrapped class throws and exception,
     * it is caught and an ElemInitError exception is thrown. The original
     * exception is lost.
     */
    ElemRefAny(const char* c_str) : Element(_hash), _val(NULL), _free(false)
    {
        if (c_str) {
            try {
                _val = new T(c_str);
                _free = true;
            } catch(...) {
                string err = "Unable to initialize element of type ";
                err += id;
                err += " with ";
                err += c_str;

                xorp_throw(ElemInitError, err);

            }

        }
	// else leave it to the default value
	else {
	    _val = new T();
	    _free = true;
        }
    }

    /**
     * Invokes the == operator on the actual classes being wrapped.
     *
     * @return whether the two values are equal.
     * @param rhs element to compare with.
     */
    bool operator==(const ElemRefAny<T>& rhs) const {
        return (*_val) == rhs.val();
    }

    /**
     * Invoke the < operator in the wrapped class.
     *
     * @return whether this is less than argument.
     * @param rhs element to compare with.
     */
    bool operator<(const ElemRefAny<T>& rhs) const {
        return (*_val) < rhs.val();
    }

    /**
     * Invokes the str() method of the actual class being wrapped.
     *
     * @return string representation of element.
     */
    string str() const { return _val->str(); }

    /**
     * @return the actual object of the class being wrapped.
     */
    const T& val() const { return *_val; }

    const char* type() const { return id; }

    ElemRefAny(const ElemRefAny<T>& copy) : Element(_hash) {
	_val = copy._val;
	_free = copy._free;
	copy._free = false;
    }

private:
    ElemRefAny& operator=(const ElemRefAny<T>&);

    const T* _val;
    mutable bool _free;
};

template<class A>
class ElemNet : public Element {
public:
    enum Mod {
	MOD_NONE,
	MOD_EXACT,
	MOD_SHORTER,
	MOD_ORSHORTER,
	MOD_LONGER,
	MOD_ORLONGER,
	MOD_NOT
    };

    static const char*	id;
    static Hash		_hash;

    ElemNet();
    ElemNet(const char*);
    ElemNet(const A&);
    ElemNet(const ElemNet<A>&);	    // copyable
    ~ElemNet();

    string	    str() const;
    const char*	    type() const;
    const A&	    val() const;
    static Mod	    str_to_mod(const char* p);
    static string   mod_to_str(Mod mod);
    BinOper&	    op() const;

    bool	operator<(const ElemNet<A>& rhs) const;
    bool	operator==(const ElemNet<A>& rhs) const;

private:
    ElemNet& operator=(const ElemNet<A>&);	// not assignable

    const A*		_net;
    Mod			_mod;
    mutable BinOper*	_op;
};

template <class A>
class ElemNextHop : public Element {
public:
    enum Var {
	VAR_NONE,
	VAR_DISCARD,
	VAR_NEXT_TABLE,
	VAR_PEER_ADDRESS,
	VAR_REJECT,
	VAR_SELF
    };

    static const char*	id;
    static Hash		_hash;

    ElemNextHop(const char*);
    ElemNextHop();
    ElemNextHop(const A& nh);

    string	str() const;
    const char*	type() const;
    Var		var() const;
    const A&	addr() const;
    const A&	val() const; // for relop compatibility

private:
    Var	_var;
    A	_addr;
};

// User defined types
typedef ElemRefAny<IPv4>		ElemIPv4;
typedef ElemAny<IPv6>			ElemIPv6;
typedef ElemAny<IPv4Range>		ElemIPv4Range;
typedef ElemAny<IPv6Range>		ElemIPv6Range;
typedef ElemNet<IPv4Net>		ElemIPv4Net;
typedef ElemNet<IPv6Net>		ElemIPv6Net;
typedef ElemAny<U32Range>		ElemU32Range;
typedef ElemNextHop<IPv4>		ElemIPv4NextHop;
typedef ElemNextHop<IPv6>		ElemIPv6NextHop;

#endif // __POLICY_COMMON_ELEMENT_HH__
