// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fti_dummy.cc,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $"

#include "fea_module.h"
#include "config.h"
#include "libxorp/xorp.h"

#include <cstdio>

#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "fti.hh"
#include "fti_dummy.hh"

bool
DummyFti::start_configuration()
{
    // No concept of starting configuration on dummy, just label up
    return mark_configuration_start();
}

bool
DummyFti::end_configuration()
{
    // No concept of ending configuration on dummy, just labelling
    return mark_configuration_end();
}

bool
DummyFti::delete_all_entries4()
{
    if (in_configuration() == false)
	return false;
   
    _t4.delete_all_nodes();
    return true;
}

bool
DummyFti::delete_entry4(const Fte4& f)
{
    if (in_configuration() == false)
	return false;

    Trie4::iterator i = _t4.find(f.net());
    if (i == _t4.end()) {
	return false;
    }
    _t4.erase(i);
    return true;
}

bool
DummyFti::add_entry4(const Fte4& f)
{
    if (in_configuration() == false)
	return false;

    int rc = _t4.route_count();
    assert(rc >= 0);

    _t4.insert(f.net(), f);
    if (_t4.route_count() == rc) {
	XLOG_WARNING("add_entry4 is over riding old entry for %s (%d %d)\n",
		     f.net().str().c_str(), rc, _t4.route_count());
    }

    return true;
}

bool
DummyFti::lookup_route4(IPv4 addr, Fte4& fte)
{
    Trie4::iterator ti = _t4.find(addr);
    if (ti != _t4.end()) {
	fte = ti.payload();
	return true;
    }
    return false;
}

bool
DummyFti::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    Trie4::iterator ti = _t4.find(dst);
    if (ti != _t4.end()) {
	fte = ti.payload();
	return true;
    }
    return false;
}

bool
DummyFti::delete_all_entries6()
{
    if (in_configuration() == false)
	return false;

    _t6.delete_all_nodes();
    return true;
}

bool
DummyFti::delete_entry6(const Fte6& f)
{
    if (in_configuration() == false)
	return false;

    Trie6::iterator i = _t6.find(f.net());
    if (i == _t6.end()) {
	return false;
    }
    _t6.erase(i);
    return true;
}

bool
DummyFti::add_entry6(const Fte6& f)
{
    if (in_configuration() == false)
	return false;
    
    int rc = _t6.route_count();
    assert(rc >= 0);

    _t6.insert(f.net(), f);
    if (_t6.route_count() == rc) {
	XLOG_WARNING("add_entry6 is over riding old entry for %s (%d %d)\n",
		     f.net().str().c_str(), rc, _t6.route_count());
    }

    return true;
}

bool
DummyFti::lookup_route6(const IPv6& addr, Fte6& fte)
{
    Trie6::iterator ti = _t6.find(addr);
    if (ti != _t6.end()) {
	fte = ti.payload();
	return true;
    }
    return false;
}

bool
DummyFti::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{
    Trie6::iterator ti = _t6.find(dst);
    if (ti != _t6.end()) {
	fte = ti.payload();
	return true;
    }
    return false;
}
