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

#ifndef __POLICY_BACKEND_POLICY_FILTER_HH__
#define __POLICY_BACKEND_POLICY_FILTER_HH__

#include "policy/common/element.hh"
#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#include "policy_instr.hh"
#include "set_manager.hh"

#include <string>
#include <map>


/**
 * @short A generic policy filter.
 *
 * It may accept/reject/modify any route which supports VarRW.
 */
class PolicyFilter {
public:
    /**
     * @short Exception thrown on configuration error.
     */
    class ConfError : public PolicyException {
    public:
	ConfError(const string& err) : PolicyException(err) {}
    };

    PolicyFilter();
    ~PolicyFilter();
    
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
     * @param os if not null, an execution trace will be output to stream. 
     */
    bool acceptRoute(VarRW& varrw, ostream* os);

private:
    vector<PolicyInstr*>* _policies;
    SetManager _sman;

    // not impl
    PolicyFilter(const PolicyFilter&);
    PolicyFilter& operator=(const PolicyFilter&);
};


#endif // __POLICY_BACKEND_POLICY_FILTER_HH__
