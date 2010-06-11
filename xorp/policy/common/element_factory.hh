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

// $XORP: xorp/policy/common/element_factory.hh,v 1.9 2008/10/02 21:58:06 bms Exp $

#ifndef __POLICY_COMMON_ELEMENT_FACTORY_HH__
#define __POLICY_COMMON_ELEMENT_FACTORY_HH__



#include <sstream>
#include "register_elements.hh"
#include "element_base.hh"
#include "elem_set.hh"
#include "policy_exception.hh"

/**
 * @short A factory for creating elements based on their type.
 *
 * Elements are created via their string represented type. They are initialized
 * via a c-style string. If this string is null, then a default value is
 * assigned to the element.
 *
 * Functions that perform the creation are registered with the factory at
 * run-time.
 *
 * An exception is throw on element creationg failure.
 *
 * Similar to Dispatcher.
 */
class ElementFactory {
public:
    // Function called to create element
    typedef Element* (*Callback)(const char*);
    
    // Container which maps a key to a callback. May consider using a hash table.
    typedef map<string,Callback> Map;

    ElementFactory();

    /**
     * @short Exception thrown if an Unknown element is being created.
     *
     * When creating an element of a type which has no registered creation
     * callback with the factory.
     */
    class UnknownElement : public PolicyException {
    public:
	UnknownElement(const char* file, size_t line, 
		       const string& init_why = "")   
            : PolicyException("UnknownElement", file, line, 
			      "ElementFactory: unable to create unknown element: " + init_why) {}  
    };

    /**
     * Register a callback with the factory.
     *
     * @param key the element id/type. Must be unique.
     * @param cb function to be called when the element must be created.
     */
    void add(const string& key, Callback cb); 

    /**
     * Create an element.
     *
     * @return the requested element. Caller is responsible for deletion.
     * @param key the type of element that needs to be created.
     * @param arg initial value of element. If null, a default is assigned.
     */
    Element* create(const string& key, const char* arg);

    /**
     * Checks whether a type exists.
     *
     * @param key the element to check for.
     * @return true if the element can be create via the factory.
     */
    static bool can_create(const string& key);

private:
    /**
     * There is only one factory map.
     *
     * Creating additional factory objects is therefore safe. No need to pass a
     * global factory around in the various classes.
     */
    static Map _map;

    /**
     * A class which registers defined callbacks upon creation.
     *
     * Callbacks are thus registered once [static] and before the factory is
     * actually used.
     */
    static RegisterElements _regelems;
};

#endif // __POLICY_COMMON_ELEMENT_FACTORY_HH__
