// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_reporter.cc,v 1.2 2007/05/03 18:46:27 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "ifconfig_reporter.hh"
#include "iftree.hh"

//
// Mechanism for reporting network interface related information to
// interested parties.
//

IfConfigUpdateReporterBase::IfConfigUpdateReporterBase(
    IfConfigUpdateReplicator& update_replicator)
    : _update_replicator(update_replicator),
      _observed_iftree(_update_replicator.observed_iftree())
{
}

IfConfigUpdateReporterBase::IfConfigUpdateReporterBase(
    IfConfigUpdateReplicator& update_replicator,
    const IfTree& observed_iftree)
    : _update_replicator(update_replicator),
      _observed_iftree(observed_iftree)
{
}

void
IfConfigUpdateReporterBase::add_to_replicator()
{
    _update_replicator.add_reporter(this);
}

void
IfConfigUpdateReporterBase::remove_from_replicator()
{
    _update_replicator.remove_reporter(this);
}


// ----------------------------------------------------------------------------
// IfConfigUpdateReplicator

IfConfigUpdateReplicator::~IfConfigUpdateReplicator()
{
}

bool
IfConfigUpdateReplicator::add_reporter(IfConfigUpdateReporterBase* rp)
{
    if (find(_reporters.begin(), _reporters.end(), rp) != _reporters.end())
	return false;
    _reporters.push_back(rp);

    //
    // Propagate all current interface information
    //
    Update update = IfConfigUpdateReporterBase::CREATED;
    IfTree::IfMap::const_iterator if_iter;

    for (if_iter = observed_iftree().ifs().begin();
	 if_iter != observed_iftree().ifs().end();
	 ++if_iter) {
	const IfTreeInterface& iface = if_iter->second;
	rp->interface_update(iface.ifname(), update);

	IfTreeInterface::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfTreeVif& vif = vif_iter->second;
	    rp->vif_update(iface.ifname(), vif.vifname(), update);

	    IfTreeVif::IPv4Map::const_iterator a4_iter;
	    for (a4_iter = vif.ipv4addrs().begin();
		 a4_iter != vif.ipv4addrs().end();
		 ++a4_iter) {
		const IfTreeAddr4& a4 = a4_iter->second;
		rp->vifaddr4_update(iface.ifname(), vif.vifname(), a4.addr(),
				    update);
	    }

	    IfTreeVif::IPv6Map::const_iterator a6_iter;
	    for (a6_iter = vif.ipv6addrs().begin();
		 a6_iter != vif.ipv6addrs().end();
		 ++a6_iter) {
		const IfTreeAddr6& a6 = a6_iter->second;
		rp->vifaddr6_update(iface.ifname(), vif.vifname(), a6.addr(),
				    update);
	    }
	}
    }
    rp->updates_completed();

    return true;
}

bool
IfConfigUpdateReplicator::remove_reporter(IfConfigUpdateReporterBase* rp)
{
    list<IfConfigUpdateReporterBase*>::iterator i = find(_reporters.begin(),
							 _reporters.end(),
							 rp);
    if (i == _reporters.end())
	return false;
    _reporters.erase(i);
    return true;
}

void
IfConfigUpdateReplicator::interface_update(const string& ifname,
					   const Update& update)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->interface_update(ifname, update);
	++i;
    }
}

void
IfConfigUpdateReplicator::vif_update(const string& ifname,
				     const string& vifname,
				     const Update& update)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vif_update(ifname, vifname, update);
	++i;
    }
}

void
IfConfigUpdateReplicator::vifaddr4_update(const string& ifname,
					  const string& vifname,
					  const IPv4&   addr,
					  const Update& update)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vifaddr4_update(ifname, vifname, addr, update);
	++i;
    }
}

void
IfConfigUpdateReplicator::vifaddr6_update(const string& ifname,
					  const string& vifname,
					  const IPv6&   addr,
					  const Update& update)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vifaddr6_update(ifname, vifname, addr, update);
	++i;
    }
}

void
IfConfigUpdateReplicator::updates_completed()
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->updates_completed();
	++i;
    }
}


// ----------------------------------------------------------------------------
// IfConfigErrorReporter methods

IfConfigErrorReporter::IfConfigErrorReporter()
{

}

void
IfConfigErrorReporter::config_error(const string& error_msg)
{
    string preamble(c_format("Config error: "));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::interface_error(const string& ifname,
				       const string& error_msg)
{
    string preamble(c_format("Interface error on %s: ", ifname.c_str()));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::vif_error(const string& ifname,
				 const string& vifname,
				 const string& error_msg)
{
    string preamble(c_format("Interface/Vif error on %s/%s: ",
			     ifname.c_str(), vifname.c_str()));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::vifaddr_error(const string& ifname,
				     const string& vifname,
				     const IPv4&   addr,
				     const string& error_msg)
{
    string preamble(c_format("Interface/Vif/Address error on %s/%s/%s: ",
			     ifname.c_str(),
			     vifname.c_str(),
			     addr.str().c_str()));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::vifaddr_error(const string& ifname,
				     const string& vifname,
				     const IPv6&   addr,
				     const string& error_msg)
{
    string preamble(c_format("Interface/Vif/Address error on %s/%s/%s: ",
			     ifname.c_str(),
			     vifname.c_str(),
			     addr.str().c_str()));
    log_error(preamble + error_msg);
}
