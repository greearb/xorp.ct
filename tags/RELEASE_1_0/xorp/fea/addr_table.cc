// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "fea/fea_module.h"
#include "libxorp/xlog.h"

#include "addr_table.hh"

AddressTableEventObserver::~AddressTableEventObserver()
{
}

AddressTableBase::~AddressTableBase()
{
}

void
AddressTableBase::add_observer(AddressTableEventObserver* o)
{
    if (o == 0)
	return;

    ObserverList::const_iterator i = find(_ol.begin(), _ol.end(), o);
    if (i == _ol.end())
	_ol.push_back(o);
}

void
AddressTableBase::remove_observer(AddressTableEventObserver* o)
{
    ObserverList::iterator i = find(_ol.begin(), _ol.end(), o);
    if (i != _ol.end())
	_ol.erase(i);
}

void
AddressTableBase::invalidate_address(const IPv4& addr, const string& why)
{
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); ++i) {
	AddressTableEventObserver* o = *i;
	o->invalidate_address(addr, why);
    }
}

void
AddressTableBase::invalidate_address(const IPv6& addr, const string& why)
{
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); ++i) {
	AddressTableEventObserver* o = *i;
	o->invalidate_address(addr, why);
    }
}
