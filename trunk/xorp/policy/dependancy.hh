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

#ifndef __POLICY_DEPENDANCY_HH__
#define __POLICY_DEPENDANCY_HH__

#include "policy/common/policy_exception.hh"

#include <list>
#include <map>
#include <string>

/**
 * @short A class which relates objects and handles dependancies between them.
 *
 * This class is a container of objects [pointers]. It relates string object
 * names to the actual objects. Also, it has the ability to add and remove
 * dependancies to that objects. A dependancy is some entity which is using a
 * specific object in the Dependancy container. This entity is string
 * represented.
 *
 * For example, if a policy x uses set y, Set y will have x added in its
 * dependancies. This means that x depends on y.
 *
 * Having a consistent dependancy list allows objects to be deleted correctly.
 */
template <class T>
class Dependancy {
public:
    // things that depend on object
    typedef list<string>	    DependancyList;
    
    // object dependancy pair.
    typedef pair<T*,DependancyList> Pair;
    
    // object of this name has these dependancies
    typedef map<string,Pair*>	    Map;

    struct ObjPair {
	const string& name;
	const T& object;

	ObjPair(const string& n, const T& o) : name(n), object(o) {}
    };
   

    /**
     * @short Exception thrown if an illegal action is requested.
     *
     * Such as deleting an object which has a non empty dependancy list.
     */
    class DependancyError : public PolicyException {
    public:
	DependancyError(const string& err) : PolicyException(err) {}
    };



    Dependancy(){}

    ~Dependancy() {
	for(typename Map::iterator i = _map.begin();
	    i != _map.end(); ++i)  {
	    
	    Pair* p = (*i).second;
	    if(p->first)
		delete p->first;
	    delete p;
	}    
    }

    /**
     * Checks if an object is present in the container.
     *
     * @return true if object is contained. False otherwise.
     * @param objectname name of the object.
     */
    bool exists(const string& objectname) const {
	return _map.find(objectname) != _map.end();
    }

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
    bool create(const string& objectname, T* object) {
	if(exists(objectname))
	    return false;
	
	Pair* p = new Pair(object,DependancyList());

	_map[objectname] = p;
	return true;
	
    }

    /**
     * Tries to remove and delete an object. Checks if object is in use [non
     * empty dependancy list].
     *
     * Throws an exception on failure.
     *
     * @param objectname object to remove and delete.
     */
    void remove(const string& objectname) {
	typename Map::iterator i = _map.find(objectname);

	if(i == _map.end())
	    throw DependancyError("Dependancy remove: Cannot find object " + 
				  objectname);
	
	Pair* p = (*i).second;

	DependancyList& s = (*p).second;

	// check if object is in use
	if(!s.empty()) {
	    // XXX: insert "by who" in message
	    throw DependancyError("Dependancy remove: Object " + objectname + 
				  " in use");
	}

	// delete object
	if(p->first)
	    delete p->first;
	delete p;

	_map.erase(i);
    }

    /**
     * Adds dependancies to this object. A dependancy is another object which
     * uses this object.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to which dependancy should be added.
     * @param dep name of object which depends on objectname.
     */
    void add_dependancy(const string& objectname, const string& dep) {
	Pair* p = findDepend(objectname);

	DependancyList& s = (*p).second;

	s.push_back(dep);
    }
    
    /**
     * Deletes a dependancy on an object.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to which dependancy should be removed.
     * @param dep name of dependancy to remove.
     */
    void del_dependancy(const string& objectname, const string& dep) {
	Pair* p = findDepend(objectname);

	DependancyList& s = (*p).second;

	s.push_back(dep);
    }

    /**
     * Returns the object being searched for
     *
     * @return object requested.
     * @param objectname name of object to return.
     */
    T& find(const string& objectname) const {
	Pair* p = findDepend(objectname);

	T* x = (*p).first;

	return *x;
    }

    /**
     * Obtains the dependancy list for an object.
     *
     * Duplicates are removed, as it is a set.
     *
     * @param objectname name of object for which dependancy list is requested.
     * @param deps set of strings filled with dependancy list.
     */
    void get_deps(const string& objectname, set<string>& deps) const {
	Pair* p = findDepend(objectname);

	DependancyList& s = (*p).second;
	for(typename DependancyList::iterator i = s.begin(); i != s.end(); ++i)
	    deps.insert(*i); // duplicates are removed [set]
    }

    /**
     * Replaces an object. The previous one is deleted.
     * Caller does not own object. Should not modify or delete it.
     *
     * Throws an exception if object does not exist.
     *
     * @param objectname name of object to replace.
     * @param obj the new object.
     */
    void update_object(const string& objectname,T* obj) {
	Pair* p = findDepend(objectname);

	// delete old
	if(p->first)
	    delete p->first;

	// replace [dependancies are maintained]
	p->first = obj;
    }

    // XXX: this interface has to be re-done...
    /**
     * Obtain an iterator for this container.
     *
     * @return iterator for Dependancy container.
     */
    typename Map::const_iterator get_iterator() const {
	return _map.begin();
    }

    /**
     * Checks if more objects are available with this iterator.
     *
     * @return true if more objects are available. False otherwise.
     * @param i iterator to use.
     */
    bool has_next(const typename Map::const_iterator& i) const {
	return i != _map.end();
    }

    /**
     * Returns the next object pair and increments the iterator.
     *
     * An object pair consists of the object name, and the actual object.
     *
     * @return the object pair associated with the iterator.
     * @param i iterator that points to object. Iterator is then incremented.
     */
    ObjPair next(typename Map::const_iterator& i) const {
	if(i == _map.end())
	    throw DependancyError("No more objects");
	

	Pair* p = (*i).second;
	
	const T* obj = p->first;

	ObjPair ret((*i).first,*obj);

	i++;

	return ret;
    }

private:
    Map _map;
    
    
    Pair* findDepend(const string& objectname) const {
	typename Map::const_iterator i = _map.find(objectname);

	if(i == _map.end())
	    throw DependancyError("Dependancy: Cannot find object of name " + 
			          objectname);

	return (*i).second;    
    }


    // not impl
    Dependancy(const Dependancy&);
    Dependancy operator=(const Dependancy&);


};

#endif // __POLICY_DEPENDANCY_HH__
