// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/config_param.hh,v 1.6 2003/08/12 08:05:33 pavlin Exp $

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
    inline const T& get() const { return (_value); }
    
    /**
     * Set the current value of the parameter.
     * 
     * @param value the current value to set the parameter to.
     */
    inline void set(const T& value) {
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
    inline ConfigParam& operator=(const T& value) {
	set(value);
	return (*this);
    }

    /**
     * Increment Operator
     * 
     * The numerical value of this configuration parameter is incremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * incremented by one.
     */
    const T& operator++() { set(_value + 1); return (_value); }

    /**
     * Decrement Operator
     * 
     * The numerical value of this configuration parameter is decremented
     * by one.
     * 
     * @return a reference to this configuration parameter after it was
     * decremented by one.
     */
    const T& operator--() { set(_value - 1); return (_value); }

    /**
     * Get the initial value of the parameter.
     */
    inline const T& get_initial_value() const { return (_initial_value); }
    
    /**
     * Reset the current value of the parameter to its initial value.
     */
    inline void reset() { set(_initial_value); }
    
private:
    T _value;			// The current value
    T _initial_value;		// The initial value
    UpdateCallback _update_cb;	// Callback invoked when _value changes
};

#endif // __LIBXORP_CONFIG_PARAM_HH__
