// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/backend/filter_base.hh,v 1.5 2008/07/23 05:11:23 pavlin Exp $

#ifndef __POLICY_BACKEND_FILTER_BASE_HH__
#define __POLICY_BACKEND_FILTER_BASE_HH__

#include "policy/common/varrw.hh"
#include <string>

/**
 * @short Base class for all policy filters.
 */
class FilterBase {
public:
    virtual ~FilterBase() {}
    
    /**
     * Configure the filter
     *
     * @param str filter configuration.
     */
    virtual void configure(const string& str) = 0;

    /**
     * Reset the filter.
     *
     * Filter becomes a NO-operation -- default action should
     * be returned everytime an acceptRoute is called.
     */
    virtual void reset() = 0;

    /**
     * See if a route is accepted by the filter.
     * The route may be modified by the filter [through VarRW].
     *
     * @return true if the route is accepted, false otherwise.
     * @param varrw the VarRW associated with the route being filtered.
     */
    virtual bool acceptRoute(VarRW& varrw) = 0;
};

#endif // __POLICY_BACKEND_FILTER_BASE_HH__
