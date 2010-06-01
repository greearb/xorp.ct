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

// $XORP: xorp/policy/backend/policy_filters.hh,v 1.11 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_FILTERS_HH__
#define __POLICY_BACKEND_POLICY_FILTERS_HH__



#include "policy_filter.hh"
#include "policy/common/filter.hh"
#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"

/**
 * @short A container for all policy filters a protocol should support.
 *
 * Filters which are not used should just not be configured / executed. In the
 * future an option to disable a filter should be added. Although, not running a
 * filter is harmless for now [if configured however, state consumes memory].
 */
class PolicyFilters :
    public NONCOPYABLE
{
public:
    class PolicyFiltersErr : public PolicyException {
    public:
        PolicyFiltersErr(const char* file, size_t line, 
			   const string& init_why = "")   
	: PolicyException("PolicyFiltersErr", file, line, init_why) {}  
    };

    PolicyFilters();
    PolicyFilters(FilterBase* im, FilterBase* sm, FilterBase* ex);
    virtual ~PolicyFilters();

    /**
     * Run a filter and decide whether route should be accepted.
     *
     * May throw an exception on run-time errors.
     *
     * @return true if route is accepted, false otherwise.
     * @param type which filter should be executed.
     * @param varrw the VarRW associated with the route to be filtered.
     */
    bool run_filter(const uint32_t& type, VarRW& varrw);

    /**
     * Configure a filter.
     *
     * Throws an exception on error.
     *
     * @param type the filter to configure.
     * @param conf the configuration of the filter.
     */
    void configure(const uint32_t& type, const string& conf);

    /**
     * Reset a filter.
     *
     * @param type the filter to reset.
     */
    void reset(const uint32_t& type);

private:
    /**
     * Decide which filter to run based on its type.
     *
     * Throws exception if ftype is invalid.
     *
     * @return filter to execute.
     * @param ftype integral filter identifier.
     */
    FilterBase&   whichFilter(const uint32_t& ftype);

private:
    FilterBase*   _import_filter;
    FilterBase*   _export_sm_filter;
    FilterBase*   _export_filter;
};

#endif // __POLICY_BACKEND_POLICY_FILTERS_HH__
