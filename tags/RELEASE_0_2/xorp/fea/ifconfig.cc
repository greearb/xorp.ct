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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $"

#include "ifconfig.hh"

static bool
map_changes(const IfTreeItem::State&		fci,
	    IfConfigUpdateReporterBase::Update& u)
{
    switch (fci) {
    case IfTreeItem::NO_CHANGE:
	return false;
    case IfTreeItem::CREATED:
	u = IfConfigUpdateReporterBase::CREATED;
	break;
    case IfTreeItem::DELETED:
	u = IfConfigUpdateReporterBase::DELETED;
	break;
    case IfTreeItem::CHANGED:
	u = IfConfigUpdateReporterBase::CHANGED;
	break;
    default:
	abort();
    }
    return true;
}

void
IfConfig::report_update(const IfTreeInterface& fi)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u))
	_ur.interface_update(fi.ifname(), u);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fv.state(), u))
	_ur.vif_update(fi.ifname(), fv.vifname(), u);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr4&	fa)

{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u))
	_ur.vifaddr4_update(fi.ifname(), fv.vifname(), fa.addr(), u);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr6&	fa)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u))
	_ur.vifaddr6_update(fi.ifname(), fv.ifname(), fa.addr(), u);
}

void
IfConfig::report_updates(const IfTree& it)
{
    //
    // Walk config looking for changes to report
    //
    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {

	const IfTreeInterface& interface = ii->second;
	report_update(interface);

	IfTreeInterface::VifMap::const_iterator vi;
	for (vi = interface.vifs().begin();
	    vi != interface.vifs().end(); ++vi) {

	    const IfTreeVif& vif = vi->second;
	    report_update(interface, vif);

	    for (IfTreeVif::V4Map::const_iterator ai = vif.v4addrs().begin();
		 ai != vif.v4addrs().end(); ai++) {
		const IfTreeAddr4& addr = ai->second;
		report_update(interface, vif, addr);
	    }

	    for (IfTreeVif::V6Map::const_iterator ai = vif.v6addrs().begin();
		 ai != vif.v6addrs().end(); ai++) {
		const IfTreeAddr6& addr = ai->second;
		report_update(interface, vif, addr);
	    }
	}
    }
}

const string&
IfConfig::push_error() const
{
    return _er.first_error();
}


/* ------------------------------------------------------------------------- */
/* SimpleIfConfigErrorReporter methods */

SimpleIfConfigErrorReporter::SimpleIfConfigErrorReporter()
    : _error_cnt(0) {}

void
SimpleIfConfigErrorReporter::interface_error(const string& ifname,
					     const string& error_msg)
{
    string preamble(c_format("On %s: ", ifname.c_str()));
    log_error(preamble + error_msg);
}

void
SimpleIfConfigErrorReporter::vif_error(const string& ifname,
				       const string& vifname,
				       const string& error_msg)
{
    string preamble(c_format("On %s/%s: ", ifname.c_str(), vifname.c_str()));
    log_error(preamble + error_msg);
}

void
SimpleIfConfigErrorReporter::vifaddr_error(const string& ifname,
					   const string& vifname,
					   const IPv4&   addr,
					   const string& error_msg)
{
    string preamble(c_format("On %s/%s/%s: ",
			     ifname.c_str(),
			     vifname.c_str(),
			     addr.str().c_str()));
    log_error(preamble + error_msg);
}

void
SimpleIfConfigErrorReporter::vifaddr_error(const string& ifname,
					   const string& vifname,
					   const IPv6&   addr,
					   const string& error_msg)
{
    string preamble(c_format("On %s/%s/%s: ",
			     ifname.c_str(),
			     vifname.c_str(),
			     addr.str().c_str()));
    log_error(preamble + error_msg);
}

