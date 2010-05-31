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

// $XORP: xorp/policy/filter_manager_base.hh,v 1.8 2008/10/02 21:57:58 bms Exp $

#ifndef __POLICY_FILTER_MANAGER_BASE_HH__
#define __POLICY_FILTER_MANAGER_BASE_HH__

#include "code.hh"

/**
 * @short Base class for a FilterManager.
 *
 * A filter manager is the entitiy which interacts with the actual policy
 * filters in protocols and the rib.
 *
 * It is used by the configuration class to trigger updates
 *
 */
class FilterManagerBase {
public:
    virtual ~FilterManagerBase() {}

    /**
     * Update a specific policy filter.
     *
     * @param t The target to update [protocol/filter pair].
     *
     */
    virtual void update_filter(const Code::Target& t) = 0;

    /**
     * Commit all updates after msec milliseconds.
     *
     * @param msec Milliseconds after which all updates should be commited.
     *
     */
    virtual void flush_updates(uint32_t msec) = 0;
};

#endif // __POLICY_FILTER_MANAGER_BASE_HH__
