// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_addr_table.cc,v 1.3 2004/02/19 00:28:45 hodson Exp $"

#include <algorithm>

#include "fea/fea_module.h"
#include "libxorp/xlog.h"

#include "ifconfig_addr_table.hh"

IfConfigAddressTable::IfConfigAddressTable(const IfTree& iftree)
    : _iftree(iftree)
{
    get_valid_addrs(_v4addrs, _v6addrs);
}

IfConfigAddressTable::~IfConfigAddressTable()
{
}

bool
IfConfigAddressTable::address_valid(const IPv4& addr) const
{
    return _v4addrs.find(addr) != _v4addrs.end();
}

bool
IfConfigAddressTable::address_valid(const IPv6& addr) const
{
    return _v6addrs.find(addr) != _v6addrs.end();
}

uint32_t
IfConfigAddressTable::address_pif_index(const IPv4& addr) const
{
    // XXX This info should be cached...
    IfTree::IfMap::const_iterator ii = _iftree.ifs().begin();
    for (; ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& iti = ii->second;
	if (iti.state() == IfTreeItem::DELETED)
	    continue;
	IfTreeInterface::VifMap::const_iterator vi = iti.vifs().begin();
	for (; vi != iti.vifs().end(); ++vi) {
	    const IfTreeVif& itv = vi->second;
	    if (itv.state() == IfTreeItem::DELETED)
		continue;
	    IfTreeVif::V4Map::const_iterator ai4 = itv.v4addrs().begin();
	    for (; ai4 != itv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& ita = ai4->second;
		if (ita.state() == IfTreeItem::DELETED)
		    continue;
		if (ita.addr() == addr)
		    return itv.pif_index();
	    }
	}
    }
    return 0;
}

uint32_t
IfConfigAddressTable::address_pif_index(const IPv6& addr) const
{
    // XXX This info should be cached...
    IfTree::IfMap::const_iterator ii = _iftree.ifs().begin();
    for (; ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& iti = ii->second;
	if (iti.state() == IfTreeItem::DELETED)
	    continue;
	IfTreeInterface::VifMap::const_iterator vi = iti.vifs().begin();
	for (; vi != iti.vifs().end(); ++vi) {
	    const IfTreeVif& itv = vi->second;
	    if (itv.state() == IfTreeItem::DELETED)
		continue;
	    IfTreeVif::V6Map::const_iterator ai6 = itv.v6addrs().begin();
	    for (; ai6 != itv.v6addrs().end(); ++ai6) {
		const IfTreeAddr6& ita = ai6->second;
		if (ita.state() == IfTreeItem::DELETED)
		    continue;
		if (ita.addr() == addr)
		    return itv.pif_index();
	    }
	}
    }

    return 0;
}

void
IfConfigAddressTable::get_valid_addrs(set<IPv4>& v4s, set<IPv6>& v6s)
{
    IfTreeItem::State deleted = IfTreeItem::DELETED;
    IfTree::IfMap::const_iterator ii = _iftree.ifs().begin();

    for (; ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& iti = ii->second;
	if (iti.enabled() == false || iti.state() == deleted)
	    continue;

	IfTreeInterface::VifMap::const_iterator vi = iti.vifs().begin();
	for (; vi != iti.vifs().end(); ++vi) {
	    const IfTreeVif& itv = vi->second;
	    if (itv.enabled() == false || itv.state() == deleted)
		continue;

	    IfTreeVif::V4Map::const_iterator ai4 = itv.v4addrs().begin();
	    for (; ai4 != itv.v4addrs().end(); ++ai4) {
		const IfTreeAddr4& ita = ai4->second;
		if (ita.enabled() == false || ita.state() == deleted)
		    continue;
		v4s.insert(ita.addr());
	    }
	    IfTreeVif::V6Map::const_iterator ai6 = itv.v6addrs().begin();
	    for (; ai6 != itv.v6addrs().end(); ++ai6) {
		const IfTreeAddr6& ita = ai6->second;
		if (ita.enabled() == false || ita.state() == deleted)
		    continue;
		v6s.insert(ita.addr());
	    }
	}
    }
}

void
IfConfigAddressTable::set_addrs(const set<IPv4>& new_v4s)
{
    //
    // Find what's in supplied interface address that's not in
    // instance address set and announce removal missing items.
    //
    // NB A separate iterator for doing the announcement would be more
    // efficient here, rather than outputting to another set and
    // iterating.
    //
    set<IPv4> delta;
    set_difference(
		   new_v4s.begin(), new_v4s.end(),
		   _v4addrs.begin(), _v4addrs.end(),
		   insert_iterator<set<IPv4> >(delta, delta.begin())
		   );

    //
    // Announce invalidated addresses
    //
    string reason("Address or parent disabled or removed");
    for (set<IPv4>::const_iterator i = delta.begin(); i != delta.end(); ++i) {
	invalidate_address(*i, reason);
    }

    //
    // Set instance addresses to supplied addresses
    //
    _v4addrs = new_v4s;
}

void
IfConfigAddressTable::set_addrs(const set<IPv6>& new_v6s)
{
    //
    // Find what's in supplied interface address that's not in
    // instance address set and announce removal missing items.
    //
    // NB A separate iterator for doing the announcement would be more
    // efficient here, rather than outputting to another set and
    // iterating.
    //
    set<IPv6> delta;
    set_difference(
		   new_v6s.begin(), new_v6s.end(),
		   _v6addrs.begin(), _v6addrs.end(),
		   insert_iterator<set<IPv6> >(delta, delta.begin())
		   );

    //
    // Announce invalidated addresses
    //
    string reason("Address or parent disabled or removed");
    for (set<IPv6>::const_iterator i = delta.begin(); i != delta.end(); ++i) {
	invalidate_address(*i, reason);
    }

    //
    // Set instance addresses to supplied addresses
    //
    _v6addrs = new_v6s;
}

inline void
IfConfigAddressTable::update()
{
    set<IPv4> v4s;
    set<IPv6> v6s;

    get_valid_addrs(v4s, v6s);
    set_addrs(v4s);
    set_addrs(v6s);
}

void
IfConfigAddressTable::interface_update(const string&	/* ifname */,
				       const Update&	/* update */,
				       bool		/* system */)
{
    update();
}

void
IfConfigAddressTable::vif_update(const string&	/* ifname */,
				 const string&	/* vifname */,
				 const Update&	/* update */,
				 bool		/* system */)
{
    update();
}

void
IfConfigAddressTable::vifaddr4_update(const string&	/* ifname */,
				      const string&	/* vifname */,
				      const IPv4&	/* addr */,
				      const Update&	/* update */,
				      bool		/* system */)
{
    update();
}

void
IfConfigAddressTable::vifaddr6_update(const string&	/* ifname */,
				      const string&	/* vifname */,
				      const IPv6&	/* addr */,
				      const Update&	/* update */,
				      bool		/* system */)
{
    update();
}

void
IfConfigAddressTable::updates_completed(bool		/* system */)
{
    update();
}
