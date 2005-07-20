// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/policy/backend/version_filter.cc,v 1.1 2005/07/20 01:29:23 abittau Exp $"

#include "policy/policy_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "version_filter.hh"
#include "policy/common/element.hh"
#include <typeinfo>

VersionFilter::VersionFilter(const string& fname) : _filter(new PolicyFilter), _fname(fname)
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
    } catch(PolicyException e) {
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
try {    
    const ElemFilter& ef = dynamic_cast<const ElemFilter&>(varrw.read(_fname));
    RefPf filter = ef.val();

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
} catch(const bad_cast& exp) {
    const Element& e = varrw.read(_fname);

    XLOG_FATAL("Reading %s but didn't get ElemFilter! Got %s: (%s)", 
	       _fname.c_str(), e.type().c_str(), e.str().c_str());
    throw PolicyException("Reading filter but didn't get ElemFilter!");
}

}
