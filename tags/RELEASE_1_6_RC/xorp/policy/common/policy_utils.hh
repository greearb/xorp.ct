// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/common/policy_utils.hh,v 1.10 2008/07/23 05:11:27 pavlin Exp $

#ifndef __POLICY_COMMON_POLICY_UTILS_HH__
#define __POLICY_COMMON_POLICY_UTILS_HH__

#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <sstream>
#include "policy_exception.hh"


/**
 * Some generic utility functions used by different policy components.
 */
namespace policy_utils {

/**
 * @short Generic exception for errors
 */
class PolicyUtilsErr : public PolicyException {
public:
    PolicyUtilsErr(const char* file, size_t line, const string& init_why = "")   
        : PolicyException("PolicyUtilsErr", file, line, init_why) {}
};


/**
 * Deletes a vector by deleting objects and deleting the vector itself.
 * It checks if objects are null and skips them if so. Also checks if vector
 * itself is null.
 *
 * @param v vector to delete
 */
template <class T>
void 
delete_vector(vector<T*>* v) {
    if(!v)
	return;

    for(typename vector<T*>::iterator i = v->begin();
	i != v->end(); ++i) {

	if(*i)
	    delete *i;
    }
    delete v;
}

/**
 * Deletes objects in container.
 * Checks if objects are null and skips them if so.
 *
 * The container is cleared at the end.
 *
 * @param l container to be deleted and cleared.
 */
template <class T>
void 
clear_container(T& l) {
    for(typename T::iterator i = l.begin();
	i != l.end(); ++i) {

	if(*i)
	    delete *i;
    }

    l.clear();	
}


/**
 * Delets objects of a map and clears the map at the end.
 * Checks for null objects.
 *
 * @param m map to be deleted and cleared.
 */
template <class A, class T>
void 
clear_map(map<A,T*>& m) {
    for(typename map<A,T*>::iterator i = m.begin();
	i != m.end(); ++i)  {

	if((*i).second)
	    delete (*i).second;
    }	
    m.clear();
}

/**
 * Delets objects of a map-like container and clears the map at the end.
 * Checks for null objects.
 *
 * @param m map-like container to be deleted and cleared.
 */
template <class T>
void 
clear_map_container(T& m) {
    for(typename T::iterator i = m.begin();
	i != m.end(); ++i)  {

	if((*i).second)
	    delete (*i).second;
    }	
    m.clear();
}

/**
 * Converts a string in the form "1,2,...,n" to a list of strings.
 *
 * @param in input string.
 * @param out output list.
 */
void str_to_list(const string& in, list<string>& out); 

/**
 * Converts a string in the form "1,2,...,n" to a set of strings.
 *
 * @param in input string.
 * @param out output set.
 */
void str_to_set(const string& in, set<string>& out); 


/**
 * Converts an object to a string via an ostringstream.
 *
 * @return string representation of object.
 * @param x object to convert to string.
 */
template <class T>
string
to_str(T x) {
    ostringstream oss;
    oss << x;
    return oss.str();
}


/**
 * Reads a file into a string.
 * An exception is thrown on error.
 *
 * @param fname filename to read.
 * @param out output string which will be filled with file content.
 */
void read_file(const string& fname, string& out);

/**
 * Count the occurences of newlines in the c-style string.
 *
 * @return the number of new lines in the string.
 * @param x the 0 terminated c-style string to count new lines in.
 */
unsigned count_nl(const char* x);

/**
 * Match a regex.
 *
 * @param str input string to check.
 * @param reg regular expression used for matching.
 * @return true if string matches regular expression
 */
bool regex(const string& str, const string& reg);

};


#endif // __POLICY_COMMON_POLICY_UTILS_HH__
