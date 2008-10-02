// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/backend/policy_filters.cc,v 1.12 2008/10/01 23:44:26 pavlin Exp $"

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
