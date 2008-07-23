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

// $XORP: xorp/policy/backend/version_filter.hh,v 1.5 2008/01/04 03:17:17 pavlin Exp $

#ifndef __POLICY_BACKEND_VERSION_FILTER_HH__
#define __POLICY_BACKEND_VERSION_FILTER_HH__

#include "policy/common/varrw.hh"
#include "filter_base.hh"
#include "policy_filter.hh"

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
class VersionFilter : public FilterBase {
public:
    /**
     * @param fname the variable to read/write in order to access filter.
     */
    VersionFilter(const VarRW::Id& fname);
    ~VersionFilter();
    
    /**
     * Configure the filter
     *
     * @param str filter configuration.
     */
    void configure(const string& str);

    /**
     * Reset the filter.
     *
     * Filter becomes a NO-operation -- default action should
     * be returned everytime an acceptRoute is called.
     */
    void reset();

    /**
     * See if a route is accepted by the filter.
     * The route may be modified by the filter [through VarRW].
     *
     * @return true if the route is accepted, false otherwise.
     * @param varrw the VarRW associated with the route being filtered.
     */
    bool acceptRoute(VarRW& varrw);

private:
    RefPf _filter;
    VarRW::Id _fname;
};

#endif // __POLICY_BACKEND_VERSION_FILTER_HH__
