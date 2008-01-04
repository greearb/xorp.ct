// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/backend/policy_filters.cc,v 1.9 2007/02/16 22:46:59 pavlin Exp $"

#include "libxorp/xorp.h"
#include "policy_filters.hh"


PolicyFilters::PolicyFilters()
{
    _import_filter = new PolicyFilter();
    _export_sm_filter = new PolicyFilter();
    _export_filter = new PolicyFilter();
}

PolicyFilters::PolicyFilters(FilterBase* im, FilterBase* sm, FilterBase* ex) :
			     _import_filter(im), _export_sm_filter(sm),
			     _export_filter(ex)
{
}

PolicyFilters::~PolicyFilters()
{
    delete _import_filter;
    delete _export_sm_filter;
    delete _export_filter;
}

bool
PolicyFilters::run_filter(const uint32_t& ftype, VarRW& varrw)
{
    FilterBase& pf = whichFilter(ftype);
    return pf.acceptRoute(varrw);
}

void
PolicyFilters::configure(const uint32_t& ftype, const string& conf)
{
    FilterBase& pf = whichFilter(ftype);
    pf.configure(conf);
}

void
PolicyFilters::reset(const uint32_t& ftype)
{
    FilterBase& pf = whichFilter(ftype);
    pf.reset();
}

FilterBase& 
PolicyFilters::whichFilter(const uint32_t& ftype)
{
    switch(ftype) {
	case 1:
	    return *_import_filter;
	case 2:
	    return *_export_sm_filter;
	case 4:
	    return *_export_filter;
	
    }
    xorp_throw(PolicyFiltersErr, 
	       "Unknown filter: " + policy_utils::to_str(ftype));
}
