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

// $XORP: xorp/policy/backend/policy_filters.hh,v 1.9 2008/07/23 05:11:23 pavlin Exp $

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
class PolicyFilters {
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

    FilterBase*   _import_filter;
    FilterBase*   _export_sm_filter;
    FilterBase*   _export_filter;

    // not impl
    PolicyFilters(const PolicyFilters&);
    PolicyFilters& operator=(const PolicyFilters&);
};

#endif // __POLICY_BACKEND_POLICY_FILTERS_HH__
