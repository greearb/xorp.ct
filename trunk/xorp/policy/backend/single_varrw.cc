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

#ident "$XORP: xorp/policy/backend/single_varrw.cc,v 1.2 2004/10/04 17:55:06 abittau Exp $"

#include "config.h"
#include "single_varrw.hh"
#include "policy/common/elem_null.hh"

SingleVarRW::SingleVarRW() : _did_first_read(false) 
{
}


SingleVarRW::~SingleVarRW() {
    policy_utils::clear_container(_trash);
}

const Element&
SingleVarRW::read(const string& id) {

    // Maybe there was a write before a read for this variable, if so, just
    // return the value... no need to bother the client.
    Map::iterator i = _map.find(id);

    // nope... no value found.
    if(i == _map.end()) {

	// if it's the first read, inform the client.
	if(!_did_first_read) {
	    start_read();
	    _did_first_read = true;
	}
	
	// the client may have initialized the variables after the start_read
	// marker, so try reading again...
	i = _map.find(id);

	// out of luck...
	if(i == _map.end())
	    throw SingleVarRWErr("Unable to read variable " + id);
    }

    const Element* e = (*i).second;

    return *e;
}

void
SingleVarRW::write(const string& id, const Element& e) {
    // XXX no paranoid checks on what we write

    _map[id] = &e;
    _modified.insert(id);
}

void
SingleVarRW::sync() {

    // nothing was changed so do not even bother the derived class
    if(_modified.empty()) {
	policy_utils::clear_container(_trash);
	return;
    }	

    // Alert the derived class we are starting commits.
    start_write();
    
    // commit the change for each element 
    // [once per element... thats where the ugly name of this class comes from]
    for(set<string>::iterator i = _modified.begin();
	i != _modified.end(); ++i) {
   
	Map::iterator iter = _map.find(*i);

	// should never occur [assert might be better]
	if(iter == _map.end())
	    throw SingleVarRWErr("Trying to sync an inexistant variable " 
				 + *i);
	
	const Element* e = (*iter).second;

	// commit the variable
	single_write(*i,*e);
    }
    
    // done commiting [so the derived class may sync]
    end_write();

    // delete all garbage
    policy_utils::clear_container(_trash);

    // yes... can commit only once.
    _modified.clear();
}

void
SingleVarRW::initialize(const string& id, Element* e) {

    // check if we already have a value for a variable.
    // if so, do nothing.
    //
    // Consider clients initializing variables on start_read.
    // Consider a variable being written to before any reads. In such a case, the
    // SingleVarRW will already have the correct value for that variable, so we
    // need to ignore any initialize() called for that variable.
    if(_map.find(id) != _map.end()) {
	if(e)
	    delete e;
	return;
    }

    // special case nulls [for supported variables, but not present in this
    // particular case].
    if(!e)
	e = new ElemNull();
    
    _map[id] = e;

    // we own the pointers.
    _trash.insert(e);
}
