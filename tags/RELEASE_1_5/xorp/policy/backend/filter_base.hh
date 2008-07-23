// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/policy/backend/filter_base.hh,v 1.4 2008/01/04 03:17:15 pavlin Exp $

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
