// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __POLICY_COMMON_ELEMENT_FACTORY_HH__
#define __POLICY_COMMON_ELEMENT_FACTORY_HH__

#include <map>
#include <string>
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
	UnknownElement(const string& elem) : 
	    PolicyException("ElementFactory: unable to create unknown element: " + elem) {}
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
     * Create a set from an STL set. 
     *
     * The objects contained in the set are converted to a string representation
     * via an ostringstream.
     *
     * @return the requested ElemSet.
     * @param s STL set to be converted to ElemSet.
     */
    template<class T>
    Element* createSet(const set<T>& s) {
	set<string> ss;

	for(typename set<T>::iterator i = s.begin();
	    i != s.end(); ++i) {

	    ostringstream oss;
	    
	    oss << *i;

	    ss.insert(oss.str());
	}

	return new ElemSet(ss);
    }
    

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
