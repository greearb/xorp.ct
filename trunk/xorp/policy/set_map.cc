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

#ident "$XORP$"

#include "policy_module.h"
#include "config.h"

#include "set_map.hh"    

const Element&
SetMap::getSet(const string& name) const {
    return _deps.find(name);
}

void
SetMap::create(const string& name) {
    // initially, set is empty [null object is fine].
    Element* e = NULL;
    if(!_deps.create(name,e))
	throw SetMapError("Can't create set " + name + " : exists");
}

void 
SetMap::update_set(const string& name, const string& elements, 
    set<string>& modified) {

    // create the object, _deps will own it...
    Element* e = _ef.create(ElemSet::id,elements.c_str());

    // see affected policies
    _deps.get_deps(name,modified);

    // replace with new set
    _deps.update_object(name,e);

}

void 
SetMap::delete_set(const string& name) {
    _deps.remove(name);
}

void 
SetMap::add_dependancy(const string& setname, const string& policyname) {
    _deps.add_dependancy(setname,policyname);
}

void 
SetMap::del_dependancy(const string& setname, const string& policyname) {
    _deps.del_dependancy(setname,policyname);
}

string
SetMap::str() const {
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
