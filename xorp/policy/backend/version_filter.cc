// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#ifndef XORP_USE_USTL
#include <typeinfo>
#endif

#include "policy/common/elem_filter.hh"

#include "version_filter.hh"


VersionFilter::VersionFilter(const VarRW::Id& fname) : 
		    _filter(new PolicyFilter), 
		    _fname(fname)
{
}

VersionFilter::~VersionFilter()
{
}

void
VersionFilter::configure(const string& conf)
{
    PolicyFilter* pf = new PolicyFilter();

    try {
	pf->configure(conf);
    // XXX: programming question:
    // Since i'm deleting pf... do i need to copy the exception [i.e. not ref to
    // exception?]
    } catch(PolicyException& e) {
	delete pf;
	throw e;
    }
    
    _filter = RefPf(pf);
}

void
VersionFilter::reset()
{
    PolicyFilter* pf = new PolicyFilter();
    pf->reset();

    _filter = RefPf(pf);
}

bool
VersionFilter::acceptRoute(VarRW& varrw)
{
    // get the associated filter
    RefPf filter;
    try {    
	const ElemFilter& ef = dynamic_cast<const ElemFilter&>(varrw.read(_fname));
	filter = ef.val();
    } catch(const bad_cast& exp) {
	const Element& e = varrw.read(_fname);
	UNUSED(e); // in case XLOG_FATAL is compiled out.

	XLOG_FATAL("Reading %d but didn't get ElemFilter! Got %s: (%s)", 
		   _fname, e.type(), e.str().c_str());
	xorp_throw(PolicyException, "Reading filter but didn't get ElemFilter!");
    }

    // filter exists... run it
    if(!filter.is_empty())
	return filter->acceptRoute(varrw);

    // assign it latest filter
    ElemFilter cur(_filter);
    // XXX for some reason varrw.write(_fname, ElemFilter(_filter)) won't
    // work... i thought it would create a tmp var on the stack...
    varrw.write(_fname, cur);

    XLOG_ASSERT(!_filter.is_empty());
    return _filter->acceptRoute(varrw);
}
