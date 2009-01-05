// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/policy/backend/set_manager.cc,v 1.11 2008/10/02 21:58:04 bms Exp $"

#include "libxorp/xorp.h"

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
	xorp_throw(SetNotFound, "No sets initialized");

    SetMap::iterator i = _sets->find(setid);
    if(i == _sets->end())
        xorp_throw(SetNotFound, "Set not found: " + setid);

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
