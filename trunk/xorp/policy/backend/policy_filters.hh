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

// $XORP$

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
	PolicyFiltersErr(const string& err) : PolicyException(err) {}
    };

    PolicyFilters();

    /**
     * Run a filter and decide whether route should be accepted.
     *
     * May throw an exception on run-time errors.
     *
     * @return true if route is accepted, false otherwise.
     * @param type which filter should be executed.
     * @param varrw the VarRW associated with the route to be filtered.
     * @param os if not null, an execution trace will be output to stream.
     */
    bool run_filter(const uint32_t& type, VarRW& varrw, ostream* os);

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
    PolicyFilter&   whichFilter(const uint32_t& ftype);

    PolicyFilter   _import_filter;
    PolicyFilter   _export_sm_filter;
    PolicyFilter   _export_filter;

    // not impl
    PolicyFilters(const PolicyFilters&);
    PolicyFilters& operator=(const PolicyFilters&);
};

#endif // __POLICY_BACKEND_POLICY_FILTERS_HH__
