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

#ident "$XORP: xorp/policy/backend/policy_filter.cc,v 1.13 2008/07/23 05:11:23 pavlin Exp $"

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

PolicyFilter::PolicyFilter() : _policies(NULL), _profiler_exec(NULL)
{
    _exec.set_set_manager(&_sman);
}

void PolicyFilter::configure(const string& str) 
{
    vector<PolicyInstr*>* policies = new vector<PolicyInstr*>();
    map<string,Element*>* sets = new map<string,Element*>();
    string err;
    
    // do the actual parsing
    if(policy_backend_parse(*policies, *sets, str, err)) {
	// get rid of temporary parse junk.
	delete_vector(policies);
	clear_map(*sets);
	delete sets;
	xorp_throw(ConfError, err);
    }

    // properly erase old conf
    reset();

    // replace with new conf
    _policies = policies;
    _sman.replace_sets(sets);
    _exec.set_policies(_policies);
}

PolicyFilter::~PolicyFilter()
{
    reset();
}

void PolicyFilter::reset()
{
    if(_policies) {
	delete_vector(_policies);
	_policies = NULL;
	_exec.set_policies(NULL);
    }
    _sman.clear();
}

bool PolicyFilter::acceptRoute(VarRW& varrw)
{
    bool default_action = true;

    // no configuration done yet.
    if(!_policies) {
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
    ostringstream os;
    IvExec::FlowAction fa = _exec.run(&varrw, &os);

    // print any trace data...
    uint32_t level = varrw.trace();
    if (varrw.trace_allowed() && level) {
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
	    trace += os.str();
	    trace += "End of trace\n";
	}
	XLOG_TRACE(true, "Policy filter result: %s", trace.c_str());
    }

    // decide what to do
    switch(fa) {
        case IvExec::REJ:
	    return false;
	    
        case IvExec::DEFAULT:
	    return default_action;

        case IvExec::ACCEPT:
	    return true;
    }

    // unreash [hopefully]
    return default_action;
}

void
PolicyFilter::set_profiler_exec(PolicyProfiler* profiler)
{
    _profiler_exec = profiler;
}
