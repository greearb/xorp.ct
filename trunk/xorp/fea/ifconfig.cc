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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.18 2003/10/28 19:52:50 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <net/if.h>

#include "ifconfig.hh"

//
// Network interfaces related configuration.
//

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
	XLOG_FATAL("Unknown IfTreeItem::State");
	break;
    }
    return true;
}

IfConfig::IfConfig(EventLoop& eventloop,
		   IfConfigUpdateReporterBase& ur,
		   IfConfigErrorReporterBase& er)
    : _eventloop(eventloop), _ur(ur), _er(er),
      _ifc_get(NULL), _ifc_set(NULL), _ifc_observer(NULL),
      _ifc_get_dummy(*this),
      _ifc_get_ioctl(*this),
      _ifc_get_sysctl(*this),
      _ifc_get_getifaddrs(*this),
      _ifc_get_proc_linux(*this),
      _ifc_get_netlink(*this),
      _ifc_set_dummy(*this),
      _ifc_set_ioctl(*this),
      _ifc_set_netlink(*this),
      _ifc_observer_dummy(*this),
      _ifc_observer_rtsock(*this),
      _ifc_observer_netlink(*this)
{

}

int
IfConfig::register_ifc_get(IfConfigGet *ifc_get)
{
    _ifc_get = ifc_get;

    return (XORP_OK);
}

int
IfConfig::register_ifc_set(IfConfigSet *ifc_set)
{
    _ifc_set = ifc_set;

    return (XORP_OK);
}

int
IfConfig::register_ifc_observer(IfConfigObserver *ifc_observer)
{
    _ifc_observer = ifc_observer;

    return (XORP_OK);
}

int
IfConfig::set_dummy()
{
    register_ifc_get(&_ifc_get_dummy);
    register_ifc_set(&_ifc_set_dummy);
    register_ifc_observer(&_ifc_observer_dummy);

    return (XORP_OK);
}

int
IfConfig::start()
{
    if (_ifc_get != NULL) {
	if (_ifc_get->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ifc_set != NULL) {
	if (_ifc_set->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ifc_observer != NULL) {
	if (_ifc_observer->start() < 0)
	    return (XORP_ERROR);
    }

    _live_config = pull_config();
    _live_config.finalize_state();

    debug_msg("Start configuration read: %s\n", _live_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");

    return (XORP_OK);
}

int
IfConfig::stop()
{
    int ret_value = XORP_OK;

    if (_ifc_observer != NULL) {
	if (_ifc_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ifc_set != NULL) {
	if (_ifc_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ifc_get != NULL) {
	if (_ifc_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    return (ret_value);
}

bool
IfConfig::push_config(const IfTree& config)
{
    if (_ifc_set == NULL)
	return false;
    return (_ifc_set->push_config(config));
}

const IfTree&
IfConfig::pull_config()
{
    // Clear the old state
    _pulled_config.clear();

    if (_ifc_get != NULL)
	_ifc_get->pull_config(_pulled_config);
    return _pulled_config;
}

void
IfConfig::report_update(const IfTreeInterface& fi,
			bool is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u))
	_ur.interface_update(fi.ifname(), u, is_system_interfaces_reportee);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			bool  is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fv.state(), u))
	_ur.vif_update(fi.ifname(), fv.vifname(), u,
		       is_system_interfaces_reportee);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr4&	fa,
			bool  is_system_interfaces_reportee)

{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u))
	_ur.vifaddr4_update(fi.ifname(), fv.vifname(), fa.addr(), u,
			    is_system_interfaces_reportee);
}

void
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr6&	fa,
			bool is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u))
	_ur.vifaddr6_update(fi.ifname(), fv.ifname(), fa.addr(), u,
			    is_system_interfaces_reportee);
}

void
IfConfig::report_updates(const IfTree& it, bool is_system_interfaces_reportee)
{
    //
    // Walk config looking for changes to report
    //
    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {

	const IfTreeInterface& interface = ii->second;
	report_update(interface, is_system_interfaces_reportee);

	IfTreeInterface::VifMap::const_iterator vi;
	for (vi = interface.vifs().begin();
	     vi != interface.vifs().end(); ++vi) {

	    const IfTreeVif& vif = vi->second;
	    report_update(interface, vif, is_system_interfaces_reportee);

	    for (IfTreeVif::V4Map::const_iterator ai = vif.v4addrs().begin();
		 ai != vif.v4addrs().end(); ai++) {
		const IfTreeAddr4& addr = ai->second;
		report_update(interface, vif, addr, is_system_interfaces_reportee);
	    }

	    for (IfTreeVif::V6Map::const_iterator ai = vif.v6addrs().begin();
		 ai != vif.v6addrs().end(); ai++) {
		const IfTreeAddr6& addr = ai->second;
		report_update(interface, vif, addr, is_system_interfaces_reportee);
	    }
	}
    }
}

const string&
IfConfig::push_error() const
{
    return _er.first_error();
}

void
IfConfig::map_ifindex(uint32_t ifindex, const string& ifname)
{
    _ifnames[ifindex] = ifname;
}

void
IfConfig::unmap_ifindex(uint32_t ifindex)
{
    IfIndex2NameMap::iterator i = _ifnames.find(ifindex);
    if (_ifnames.end() != i)
	_ifnames.erase(i);
}

const char *
IfConfig::get_ifname(uint32_t ifindex)
{
    IfIndex2NameMap::const_iterator i = _ifnames.find(ifindex);

    if (_ifnames.end() == i) {
#ifdef HAVE_IF_INDEXTONAME
	char name_buf[IF_NAMESIZE];
	const char* ifname = if_indextoname(ifindex, name_buf);

	if (ifname != NULL)
	    map_ifindex(ifindex, ifname);

	return ifname;
#else
	return NULL;
#endif // ! HAVE_IF_INDEXTONAME
    }
    return i->second.c_str();
}

uint32_t
IfConfig::get_ifindex(const string& ifname)
{
    IfIndex2NameMap::const_iterator i;

    for (i = _ifnames.begin(); i != _ifnames.end(); ++i) {
	if (i->second == ifname)
	    return i->first;
    }

#ifdef HAVE_IF_NAMETOINDEX
    uint32_t ifindex = if_nametoindex(ifname.c_str());
    if (ifindex > 0)
	map_ifindex(ifindex, ifname);
    return ifindex;
#else
    return 0;
#endif // ! HAVE_IF_NAMETOINDEX
}

IfTreeInterface *
IfConfig::get_if(IfTree& it, const string& ifname)
{
    IfTree::IfMap::iterator ii = it.get_if(ifname);

    if (it.ifs().end() == ii)
	return NULL;

    return &ii->second;
}

IfTreeVif *
IfConfig::get_vif(IfTree& it, const string& ifname, const string& vifname)
{
    IfTreeInterface* fi = get_if(it, ifname);

    if (NULL == fi)
	return NULL;

    IfTreeInterface::VifMap::iterator vi = fi->get_vif(vifname);
    if (fi->vifs().end() == vi)
	return NULL;

    return &vi->second;
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
					   const Update& update,
					   bool		 system)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->interface_update(ifname, update, system);
	++i;
    }
}

void
IfConfigUpdateReplicator::vif_update(const string& ifname,
				     const string& vifname,
				     const Update& update,
				     bool	   system)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vif_update(ifname, vifname, update, system);
	++i;
    }
}

void
IfConfigUpdateReplicator::vifaddr4_update(const string& ifname,
					  const string& vifname,
					  const IPv4&   addr,
					  const Update& update,
					  bool		system)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vifaddr4_update(ifname, vifname, addr, update, system);
	++i;
    }
}

void
IfConfigUpdateReplicator::vifaddr6_update(const string& ifname,
					  const string& vifname,
					  const IPv6&   addr,
					  const Update& update,
					  bool		system)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->vifaddr6_update(ifname, vifname, addr, update, system);
	++i;
    }
}


// ----------------------------------------------------------------------------
// IfConfigErrorReporter methods

IfConfigErrorReporter::IfConfigErrorReporter()
{

}

void
IfConfigErrorReporter::interface_error(const string& ifname,
				       const string& error_msg)
{
    string preamble(c_format("On %s: ", ifname.c_str()));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::vif_error(const string& ifname,
				 const string& vifname,
				 const string& error_msg)
{
    string preamble(c_format("On %s/%s: ", ifname.c_str(), vifname.c_str()));
    log_error(preamble + error_msg);
}

void
IfConfigErrorReporter::vifaddr_error(const string& ifname,
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
IfConfigErrorReporter::vifaddr_error(const string& ifname,
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
