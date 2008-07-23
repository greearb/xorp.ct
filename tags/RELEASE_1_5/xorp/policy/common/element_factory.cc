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

#ident "$XORP: xorp/policy/common/element_factory.cc,v 1.10 2008/01/04 03:17:18 pavlin Exp $"

#include "policy/policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "element_factory.hh"


// static members
ElementFactory::Map ElementFactory::_map;
RegisterElements ElementFactory::_regelems;

ElementFactory::ElementFactory()
{
}

void 
ElementFactory::add(const string& key, Callback cb)
{
    // it is safe to blindly replace callbacks, but probably an error ;D
    XLOG_ASSERT(_map.find(key) == _map.end());

    _map[key] = cb;
}

Element* 
ElementFactory::create(const string& key, const char* arg)
{
    Map::iterator i = _map.find(key);

    // No way of creating element
    if(i == _map.end())
	xorp_throw(UnknownElement, key);

    // execute the callback
    return (i->second)(arg);
}

bool
ElementFactory::can_create(const string& key)
{
    return _map.find(key) != _map.end();
}
