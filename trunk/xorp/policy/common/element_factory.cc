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
#include "element_factory.hh"
#include <assert.h>

// static members
ElementFactory::Map ElementFactory::_map;
RegisterElements ElementFactory::_regelems;


ElementFactory::ElementFactory() {
}

void 
ElementFactory::add(const string& key, Callback cb) {

    // it is safe to blindly replace callbacks, but probably an error ;D
    assert(_map.find(key) == _map.end());

    _map[key] = cb;
}

Element* 
ElementFactory::create(const string& key, const char* arg) {
    Map::iterator i = _map.find(key);

    // No way of creating element
    if(i == _map.end())
	throw UnknownElement(key);

    // execute the callback
    return (i->second)(arg);
}
