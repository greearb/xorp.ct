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

#include "config.h"
#include "set_manager.hh"
#include "policy/common/policy_utils.hh"

SetManager::SetManager() : _sets(NULL) {
}

SetManager::~SetManager() {
    clear();
}

const Element&
SetManager::getSet(const string& setid) const {
    if(!_sets)
	throw SetNotFound("No sets initialized");

    SetMap::iterator i = _sets->find(setid);
    if(i == _sets->end())
        throw SetNotFound("Set not found: " + setid);

    Element* e = (*i).second;

    return *e;
}

void
SetManager::replace_sets(SetMap* sets) {
    clear();

    _sets = sets;
}

void
SetManager::clear() {
    if(_sets) {
	policy_utils::clear_map(*_sets);
	delete _sets;
	_sets = NULL;
    }
}
