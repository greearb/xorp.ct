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
    PolicyUtilsErr(const string& err) : PolicyException(err) {}
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

};


#endif // __POLICY_COMMON_POLICY_UTILS_HH__
