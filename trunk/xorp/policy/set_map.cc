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

#ident "$XORP: xorp/policy/set_map.cc,v 1.10 2007/02/16 22:46:56 pavlin Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"

#include "set_map.hh"


const Element&
SetMap::getSet(const string& name) const
{
    return _deps.find(name);
}

void
SetMap::create(const string& name)
{
    // initially, set is empty [null object is fine].
    Element* e = NULL;
    if(!_deps.create(name,e))
	xorp_throw(SetMapError, "Can't create set " + name + " : exists");
}

void 
SetMap::update_set(const string& type, const string& name, 
		   const string& elements, set<string>& modified)
{
    // create the object, _deps will own it...
    Element* e = _ef.create(type, elements.c_str());

    // see affected policies
    _deps.get_deps(name, modified);

    // replace with new set
    _deps.update_object(name, e);
}

void 
SetMap::delete_set(const string& name)
{
    _deps.remove(name);
}

void
SetMap::add_to_set(const string& type, const string& name,
		   const string& element, set<string>& modified)
{
    Element* e = _deps.find_ptr(name);

    // Find the element
    if (e == NULL) {
	// First element to the set
	update_set(type, name, element, modified);
	return;
    }

    // Check the element type
    if (type != string(e->type())) {
	string error_msg = c_format("Can't add to set %s: type mismatch "
				    "(received %s expected %s)",
				    name.c_str(),
				    type.c_str(),
				    e->type());
	xorp_throw(SetMapError, error_msg);
    }

    // Get a string with the existing elements and add the new element
    string elements = e->str();
    if (! elements.empty())
	elements += ",";
    elements += element;

    update_set(type, name, elements, modified);
}

void
SetMap::delete_from_set(const string& type, const string& name,
			const string& element, set<string>& modified)
{
    Element* e = _deps.find_ptr(name);

    // Find the element
    if (e == NULL) {
	string error_msg = c_format("Can't delete from set %s: not found",
				    name.c_str());
	xorp_throw(SetMapError, error_msg);
    }

    // Check the element type
    if (type != string(e->type())) {
	string error_msg = c_format("Can't delete from set %s: type mismatch "
				    "(received %s expected %s)",
				    name.c_str(),
				    type.c_str(),
				    e->type());
	xorp_throw(SetMapError, error_msg);
    }

    // Get a string with the existing elements and delete the element
    string elements = e->str();
    set<string> s;
    policy_utils::str_to_set(elements.c_str(), s);
    s.erase(element);

    // Recreate a string with the remaining elements
    elements = "";
    set<string>::iterator i;
    for (i = s.begin(); i != s.end(); ++i) {
	if (! elements.empty())
	    elements += ",";
	elements += *i;
    }

    update_set(type, name, elements, modified);
}

void 
SetMap::add_dependancy(const string& setname, const string& policyname)
{
    _deps.add_dependancy(setname,policyname);
}

void 
SetMap::del_dependancy(const string& setname, const string& policyname)
{
    _deps.del_dependancy(setname,policyname);
}

string
SetMap::str() const
{
    Dep::Map::const_iterator i = _deps.get_iterator();

    string ret;


    while(_deps.has_next(i)) {
	Dep::ObjPair op(_deps.next(i));

	ret += op.name + ": ";
	ret += op.object.str();
	ret += "\n";
    }

    return ret;
}
