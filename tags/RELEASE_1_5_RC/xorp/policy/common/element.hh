// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/common/element.hh,v 1.11 2007/02/16 22:47:02 pavlin Exp $

#ifndef __POLICY_COMMON_ELEMENT_HH__
#define __POLICY_COMMON_ELEMENT_HH__

#include <string>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "element_base.hh"
#include "policy_exception.hh"
#include "policy_utils.hh"
#include "policy/policy_module.h"

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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemInt32() {}

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
    ElemInt32(const char* c_str) {
	if(c_str)
	    _val = strtol(c_str,NULL,10);
	else
	    _val = 0;
    }
    ElemInt32(const int32_t val) : _val(val) {}

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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemU32() {}
    ElemU32(const char* c_str) {
	if(c_str)
	    _val = strtoul(c_str,NULL,10); 
	else
	    _val = 0;
    }
    ElemU32(const uint32_t val) : _val(val) {}

    string str() const {
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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemCom32() {}
    ElemCom32(const char*);		// in element.cc
    ElemCom32(const uint32_t val) : _val(val) {}

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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemStr() {}

    ElemStr(const char* val) {
	if(val)
	    _val = val;
	else
	    _val = "";
    }

    ElemStr(const string& str) : _val(str) {}

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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemBool() {}
    ~ElemBool() {}
    ElemBool(const char* c_str) {
	if(c_str && (strcmp(c_str,"true") == 0) )
	    _val = true;
	else
	    _val = false;
    }
    ElemBool(const bool val) : _val(val) {}

    string str() const {
	if(_val)
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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }

    ElemAny() : _val() {}
    ElemAny(const T& val) : _val(val) {}

    /**
     * If the c-style constructor of the wrapped class throws and exception,
     * it is caught and an ElemInitError exception is thrown. The original
     * exception is lost.
     */
    ElemAny(const char* c_str) : _val() {
	if(c_str) {
	    try {
		_val = T(c_str);
	    } catch(...) {
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

    void set_hash(const Hash& x) {
	_hash = x;
    }
    Hash hash() const { return _hash; }
    
    ElemRefAny() : _val(new T()), _free(true) {}
    ElemRefAny(const T& val) : _val(&val), _free(false) {}
    ElemRefAny(const T& val, bool f) : _val(&val), _free(f) {}
    ~ElemRefAny() { if (_free) delete _val; }

    /**
     * If the c-style constructor of the wrapped class throws and exception,
     * it is caught and an ElemInitError exception is thrown. The original
     * exception is lost.
     */
    ElemRefAny(const char* c_str) : _val(NULL), _free(false) {
        if(c_str) {
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

    ElemRefAny(const ElemRefAny<T>& copy) : Element() {
	_val = copy._val;
	_free = copy._free;
	copy._free = false;
    }

private:
    ElemRefAny& operator=(const ElemRefAny<T>&);

    const T* _val;
    mutable bool _free;
};

// User defined types
typedef ElemRefAny<IPv4>	    ElemIPv4;
typedef ElemAny<IPv6>		    ElemIPv6;
typedef ElemAny<IPv4Range>	    ElemIPv4Range;
typedef ElemAny<IPv6Range>	    ElemIPv6Range;
typedef ElemRefAny<IPNet<IPv4> >    ElemIPv4Net;
typedef ElemAny<IPNet<IPv6> >	    ElemIPv6Net;
typedef ElemAny<U32Range>	    ElemU32Range;

#endif // __POLICY_COMMON_ELEMENT_HH__
