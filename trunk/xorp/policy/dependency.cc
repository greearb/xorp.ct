// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/dependency.cc,v 1.1 2008/08/06 08:28:01 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "dependency.hh"
#include "common/element_base.hh"
#include "policy_statement.hh"

template <class T>
Dependency<T>::Dependency()
{
}

template <class T>
Dependency<T>::~Dependency()
{
    clear();
}

template <class T>
void
Dependency<T>::clear()
{
    for (typename Map::iterator i = _map.begin(); i != _map.end(); ++i) {
        Pair* p = (*i).second;
	
	if (p->first != NULL)
	    delete p->first;

        delete p;
    }

    _map.clear();
}

template <class T>
bool
Dependency<T>::exists(const string& objectname) const
{
    return _map.find(objectname) != _map.end();
}

template <class T>
bool
Dependency<T>::create(const string& objectname, T* object)
{
    if (exists(objectname))
	return false;
	
    Pair* p = new Pair(object, DependencyList());

    _map[objectname] = p;

    return true;
}

template <class T>
void
Dependency<T>::remove(const string& objectname)
{
    typename Map::iterator i = _map.find(objectname);

    if (i == _map.end())
	xorp_throw(DependencyError,
		   "Dependency remove: Cannot find object " + objectname);
	
    Pair* p = (*i).second;

    DependencyList& s = (*p).second;

    // check if object is in use
    if (!s.empty()) {
	ostringstream oss;

	oss << "Dependency remove: Object " << objectname << " in use by: ";

	for (DependencyList::iterator j = s.begin(); j != s.end(); ++j)
	    oss << *j << " ";
		
	xorp_throw(DependencyError, oss.str());
    }

    // delete object
    if (p->first)
	delete p->first;

    delete p;

    _map.erase(i);
}

template <class T>
void
Dependency<T>::add_dependency(const string& objectname, const string& dep)
{
    Pair* p = findDepend(objectname);

    DependencyList& s = (*p).second;

    s.push_back(dep);
}

template <class T>
void
Dependency<T>::del_dependency(const string& objectname, const string& dep)
{
    Pair* p = findDepend(objectname);

    DependencyList& s = (*p).second;

    s.remove(dep);
}

template <class T>
T&
Dependency<T>::find(const string& objectname) const
{
    Pair* p = findDepend(objectname);

    T* x = (*p).first;

    return *x;
}

template <class T>
T*
Dependency<T>::find_ptr(const string& objectname) const
{
    typename Map::const_iterator i = _map.find(objectname);

    if (i == _map.end())
	return (NULL);

    Pair* p = i->second;
    T* x = (*p).first;

    return x;
}

template <class T>
void
Dependency<T>::get_deps(const string& objectname, set<string>& deps) const
{
    Pair* p = findDepend(objectname);

    DependencyList& s = (*p).second;
    for (typename DependencyList::iterator i = s.begin(); i != s.end(); ++i)
	deps.insert(*i); // duplicates are removed [set]
}

template <class T>
void
Dependency<T>::update_object(const string& objectname,T* obj)
{
    Pair* p = findDepend(objectname);

    // delete old
    if (p->first)
	delete p->first;

    // replace [dependencies are maintained]
    p->first = obj;
}

template <class T>
typename Dependency<T>::Map::const_iterator
Dependency<T>::get_iterator() const
{
    return _map.begin();
}

template <class T>
bool
Dependency<T>::has_next(const typename Map::const_iterator& i) const
{
    return i != _map.end();
}

template <class T>
typename Dependency<T>::ObjPair
Dependency<T>::next(typename Map::const_iterator& i) const
{
    if (i == _map.end())
        xorp_throw(DependencyError, "No more objects");
	
    Pair* p = (*i).second;
	
    const T* obj = p->first;

    ObjPair ret((*i).first,*obj);

    i++;

    return ret;
}

template <class T>
void
Dependency<T>::keys(KEYS& out) const
{
    typename Map::const_iterator i = get_iterator();

    while (has_next(i)) {
        ObjPair op(next(i));

        out.insert(op.name);
    }
}

template <class T>
typename Dependency<T>::Pair*
Dependency<T>::findDepend(const string& objectname) const
{
    typename Map::const_iterator i = _map.find(objectname);

    if (i == _map.end())
	xorp_throw(DependencyError,
		   "Dependency: Cannot find object of name " + objectname);

    return (*i).second;    
}

template class Dependency<Element>;
template class Dependency<PolicyStatement>;
