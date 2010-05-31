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

// $XORP: xorp/policy/backend/policy_filter.hh,v 1.13 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_FILTER_HH__
#define __POLICY_BACKEND_POLICY_FILTER_HH__

#include <string>
#include <map>

#include <boost/noncopyable.hpp>

#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#include "policy_instr.hh"
#include "set_manager.hh"
#include "filter_base.hh"
#include "iv_exec.hh"
#include "libxorp/ref_ptr.hh"

/**
 * @short A generic policy filter.
 *
 * It may accept/reject/modify any route which supports VarRW.
 */
class PolicyFilter :
    public boost::noncopyable,
    public FilterBase
{
public:
    /**
     * @short Exception thrown on configuration error.
     */
    class ConfError : public PolicyException {
    public:
        ConfError(const char* file, size_t line, const string& init_why = "")
            : PolicyException("ConfError", file, line, init_why) {}   
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
     */
    bool acceptRoute(VarRW& varrw);

    void set_profiler_exec(PolicyProfiler* profiler);

private:
    vector<PolicyInstr*>*   _policies;
    SetManager		    _sman;
    IvExec		    _exec;
    PolicyProfiler*	    _profiler_exec;
    SUBR*		    _subr;
};

typedef ref_ptr<PolicyFilter> RefPf;

#endif // __POLICY_BACKEND_POLICY_FILTER_HH__
