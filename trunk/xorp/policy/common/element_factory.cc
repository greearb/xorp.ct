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
