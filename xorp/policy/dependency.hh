// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/policy/dependency.hh,v 1.4 2008/10/02 21:57:58 bms Exp $

#ifndef __POLICY_DEPENDENCY_HH__
#define __POLICY_DEPENDENCY_HH__

#include <list>
#include <map>
#include <string>
#include <sstream>
#include <set>



#include "policy/common/policy_exception.hh"

/**
 * @short A class which relates objects and handles dependencies between them.
 *
 * This class is a container of objects [pointers]. It relates string object
 * names to the actual objects. Also, it has the ability to add and remove
 * dependencies to that objects. A dependency is some entity which is using a
 * specific object in the Dependency container. This entity is string
 * represented.
 *
 * For example, if a policy x uses set y, Set y will have x added in its
 * dependencies. This means that x depends on y.
 *
 * Having a consistent dependency list allows objects to be deleted correctly.
 */
template <class T>
class Dependency :
    public NONCOPYABLE
{
public:
    // things that depend on object
    typedef list<string>	    DependencyList;

    // object dependency pair.
    typedef pair<T*,DependencyList> Pair;

    // object of this name has these dependencies
    typedef map<string,Pair*>	    Map;

    typedef set<string>		    KEYS;

    struct ObjPair {
	const string& name;
	const T& object;

	ObjPair(const string& n, const T& o) : name(n), object(o) {}
    };
   
    /**
     * @short Exception thrown if an illegal action is requested.
     *
     * Such as deleting an object which has a non empty dependency list.
     */
    class DependencyError : public PolicyException {
    public:
        DependencyError(const char* file, size_t line, 
			const string& init_why = "")
	: PolicyException("DependencyError", file, line, init_why) {} 
    };

    Dependency();
    ~Dependency();

    void clear();

    /**
     * Checks if an object is present in the container.
     *
     * @return true if object is contained. False otherwise.
     * @param objectname name of the object.
     */
    bool exists(const string& objectname) const;

    /**
     * Attempts to create an object. If creation is successfull, the object
     * ownership is transfered to this container. The caller should not modify /
     * delete the object any more.
     *
     * If object exists, creation fails.
     *
     * @return true if creation was successful. False otherwise.
     * @param objectname name of the object.
     * @param object the actual object.
     */
    bool create(const string& objectname, T* object);

    /**
     * Tries to remove and delete an object. Checks if object is in use [non
     * empty dependency list].
     *
     * Throws an exception on failure.
     *
     * @param objectname object to remove and delete.
     */
    void remove(const string& objectname);

    /**
     * Adds dependencies to this object. A dependency is another object which
     * uses this object.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to which dependency should be added.
     * @param dep name of object which depends on objectname.
     */
    void add_dependency(const string& objectname, const string& dep);

    /**
     * Deletes a dependency on an object.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to which dependency should be removed.
     * @param dep name of dependency to remove.
     */
    void del_dependency(const string& objectname, const string& dep);

    /**
     * Returns the object being searched for.
     *
     * @param objectname name of object to return.
     * @return object requested.
     */
    T& find(const string& objectname) const;

    /**
     * Returns a pointer the object being searched for.
     *
     * @param objectname name of object to return.
     * @return a pointer to the object requested if found, otherwise NULL.
     */
    T* find_ptr(const string& objectname) const;

    /**
     * Obtains the dependency list for an object.
     *
     * Duplicates are removed, as it is a set.
     *
     * @param objectname name of object for which dependency list is requested.
     * @param deps set of strings filled with dependency list.
     */
    void get_deps(const string& objectname, set<string>& deps) const;

    /**
     * Replaces an object. The previous one is deleted.
     * Caller does not own object. Should not modify or delete it.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to replace.
     * @param obj the new object.
     */
    void update_object(const string& objectname,T* obj);

    // XXX: this interface has to be re-done...
    /**
     * Obtain an iterator for this container.
     *
     * @return iterator for Dependency container.
     */
    typename Map::const_iterator get_iterator() const;

    /**
     * Checks if more objects are available with this iterator.
     *
     * @return true if more objects are available. False otherwise.
     * @param i iterator to use.
     */
    bool has_next(const typename Map::const_iterator& i) const;

    /**
     * Returns the next object pair and increments the iterator.
     *
     * An object pair consists of the object name, and the actual object.
     *
     * @return the object pair associated with the iterator.
     * @param i iterator that points to object. Iterator is then incremented.
     */
    ObjPair next(typename Map::const_iterator& i) const;

    void keys(KEYS& out) const;

private:
    Map _map;

    Pair* findDepend(const string& objectname) const;
};

#endif // __POLICY_DEPENDENCY_HH__
