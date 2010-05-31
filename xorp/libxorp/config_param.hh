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

// $XORP: xorp/libxorp/config_param.hh,v 1.17 2008/10/02 21:57:30 bms Exp $

#ifndef __LIBXORP_CONFIG_PARAM_HH__
#define __LIBXORP_CONFIG_PARAM_HH__

#include "libxorp/callback.hh"


/**
 * @short A class for storing a configuration parameter.
 * 
 * This class can be used to store a configuration parameter.
 * Such parameter has a current and a default value.
 */
template<class T>
class ConfigParam {
public:
    typedef typename XorpCallback1<void, T>::RefPtr UpdateCallback;
    
    /**
     * Constructor of a parameter with an initial value.
     * 
     * Create a configurable parameter, and initialize its initial
     * and current value.
     * 
     * @param value the initial and current value to initialize the
     * parameter to.
     */
    explicit ConfigParam(const T& value)
	: _value(value), _initial_value(value) {}
    
    /**
     * Constructor of a parameter with an initial value and a callback.
     *
     * Create a configurable parameter, initialize it, and assign
     * a callback method that will be invoked when the value changes.
     *
     * @param value the initial and current value to initialize the
     * parameter to.
     * 
     * @param update_cb the callback method that will be invoked when the
     * value changes.
     */
    ConfigParam(const T& value, const UpdateCallback& update_cb)
	: _value(value), _initial_value(value), _update_cb(update_cb) {}
    
    /**
     * Destructor
     */
    virtual ~ConfigParam() {}
    
    /**
     * Get the current value of the parameter.
     * 
     * @return the current value of the parameter.
     */
    const T& get() const { return (_value); }
    
    /**
     * Set the current value of the parameter.
     * 
     * @param value the current value to set the parameter to.
     */
    void set(const T& value) {
	_value = value;
	if (! _update_cb.is_empty())
	    _update_cb->dispatch(_value);
    }
    
    /**
     * Assignment operator
     * 
     * @param value the value to assign to the parameter.
     * @return the parameter with the new value assigned.
     */
    ConfigParam& operator=(const T& value) {
	set(value);
	return (*this);
    }

    /**
     * Increment Operator (prefix).
     * 
     * The numerical value of this configuration parameter is incremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * incremented by one.
     */
    const T& operator++() { return (incr()); }

    /**
     * Increment Operator (postfix).
     * 
     * The numerical value of this configuration parameter is incremented
     * by one.
     * 
     * @return the value of this configuration parameter before it was
     * incremented by one.
     */
    T operator++(int) {
	T old_value = _value;
	incr();
	return (old_value);
    }

    /**
     * Increment Operator.
     * 
     * The numerical value of this configuration parameter is incremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * incremented by one.
     */
    const T& incr() { set(_value + 1); return (_value); }

    /**
     * Decrement Operator (prefix).
     * 
     * The numerical value of this configuration parameter is decremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * decremented by one.
     */
    const T& operator--() { return (decr()); }

    /**
     * Decrement Operator (postfix).
     * 
     * The numerical value of this configuration parameter is decremented
     * by one.
     * 
     * @return the value of this configuration parameter before it was
     * decremented by one.
     */
    T operator--(int) {
	T old_value = _value;
	decr();
	return (old_value);
    }

    /**
     * Decrement Operator.
     * 
     * The numerical value of this configuration parameter is decremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * decremented by one.
     */
    const T& decr() { set(_value - 1); return (_value); }

    /**
     * Get the initial value of the parameter.
     */
    const T& get_initial_value() const { return (_initial_value); }
    
    /**
     * Reset the current value of the parameter to its initial value.
     */
    void reset() { set(_initial_value); }
    
private:
    T _value;			// The current value
    T _initial_value;		// The initial value
    UpdateCallback _update_cb;	// Callback invoked when _value changes
};

#endif // __LIBXORP_CONFIG_PARAM_HH__
