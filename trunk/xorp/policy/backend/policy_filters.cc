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

#ident "$XORP$"

#include "config.h"
#include "policy_filters.hh"


PolicyFilters::PolicyFilters() {
}

bool
PolicyFilters::run_filter(const uint32_t& ftype, VarRW& varrw,
			  ostream* os) {

    PolicyFilter& pf = whichFilter(ftype);
    return pf.acceptRoute(varrw,os);
}

void
PolicyFilters::configure(const uint32_t& ftype, const string& conf) {
    
    PolicyFilter& pf = whichFilter(ftype);
    pf.configure(conf);
}

void
PolicyFilters::reset(const uint32_t& ftype) {

    PolicyFilter& pf = whichFilter(ftype);
    pf.reset();
}


PolicyFilter& 
PolicyFilters::whichFilter(const uint32_t& ftype) {
    switch(ftype) {
	case 1:
	    return _import_filter;
	case 2:
	    return _export_sm_filter;
	case 4:
	    return _export_filter;
	
    }
    throw PolicyFiltersErr("Unknown filter: " +
			   policy_utils::to_str(ftype));
}
