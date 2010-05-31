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



#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "policy/common/policy_utils.hh"
#include "policy_filter.hh"
#include "policy_backend_parser.hh"
#include "set_manager.hh"
#include "iv_exec.hh"

using namespace policy_utils;
using policy_backend_parser::policy_backend_parse;

PolicyFilter::PolicyFilter() : _policies(NULL), _profiler_exec(NULL),
			       _subr(NULL)
{
    _exec.set_set_manager(&_sman);
}

void PolicyFilter::configure(const string& str) 
{
    vector<PolicyInstr*>* policies = new vector<PolicyInstr*>();
    map<string,Element*>* sets = new map<string,Element*>();
    SUBR* subr = new SUBR;
    string err;

    // do the actual parsing
    if (policy_backend_parse(*policies, *sets, *subr, str, err)) {
	// get rid of temporary parse junk.
	delete_vector(policies);
	clear_map(*sets);
	clear_map(*subr);
	delete sets;
	delete subr;
	xorp_throw(ConfError, err);
    }

    // properly erase old conf
    reset();

    // replace with new conf
    _policies = policies;
    _subr = subr;
    _sman.replace_sets(sets);
    _exec.set_policies(_policies);
    _exec.set_subr(_subr);
}

PolicyFilter::~PolicyFilter()
{
    reset();
}

void PolicyFilter::reset()
{
    if (_policies) {
	delete_vector(_policies);
	_policies = NULL;
	_exec.set_policies(NULL);
    }

    if (_subr) {
	clear_map(*_subr);
	delete _subr;
	_subr = NULL;
    }

    _sman.clear();
}

bool PolicyFilter::acceptRoute(VarRW& varrw)
{
    bool default_action = true;

    // no configuration done yet.
    if (!_policies) {
	// need to sync.  Consider case where the parent [such as version policy
	// filter] performed a write for some reason.  If we return without
	// syncing, it might be a problem [i.e. when using singlevarrw which
	// will perform the write only on sync!]
	varrw.sync();
	return default_action;
    }	

    // setup profiling
    _exec.set_profiler(_profiler_exec);

    // run policies
    IvExec::FlowAction fa = _exec.run(&varrw);

    // print any trace data...
    uint32_t level = varrw.trace();
    if (level) {
	string trace = "";

	// basic, one line [hopefully!] info...
	if (level > 0) {
	    trace += varrw.more_tracelog();

	    switch (fa) {
		case IvExec::REJ:
		    trace += ": rejected";
		    break;
	    
		case IvExec::DEFAULT:
		    trace += ": default action";
		    break;

		case IvExec::ACCEPT:
		    trace += ": accepted";
		    break;
	    }
	}

	if (level > 1) {
	    trace += "\nBasic VarRW trace:\n";
	    trace += varrw.tracelog();
	}

	if (level > 2) {
	    trace += "Execution trace:\n";
	    trace += _exec.tracelog();
	    trace += "End of trace\n";
	}

	XLOG_TRACE(true, "Policy filter result: %s", trace.c_str());
    }

    // decide what to do
    switch (fa) {
        case IvExec::REJ:
	    return false;

        case IvExec::DEFAULT:
	    return default_action;

        case IvExec::ACCEPT:
	    return true;
    }

    // unreach [hopefully]
    return default_action;
}

void
PolicyFilter::set_profiler_exec(PolicyProfiler* profiler)
{
    _profiler_exec = profiler;
}
