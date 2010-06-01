// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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


#ifndef __POLICY_BACKEND_VERSION_FILTERS_HH__
#define __POLICY_BACKEND_VERSION_FILTERS_HH__

#include "policy_filters.hh"

/**
 * @short Policy filters which support versioning [i.e. keep old version].
 *
 * The idea is to create a new policy filter on each configuration.  Whenever a
 * route is being processed, you read which filter to run.  If this filter is 0,
 * [null pointer] then give it the last configuration.  Else just run whatever
 * filter is returned.
 *
 * Filters should be referenced counted by routes.  When reference count reaches
 * 0, it should be deleted.
 *
 * Why not keep filters internally here and read a filter id from route?  Well
 * because we cannot assume when to increment and decrement the reference count.
 * Say it's a normal route lookup and we do the filtering, and it results to
 * "accepted".  It doesn't imply we need to +1 the reference count.
 */
class VersionFilters : public PolicyFilters {
public:
    VersionFilters();
};

#endif // __POLICY_BACKEND_VERSION_FILTERS_HH__
