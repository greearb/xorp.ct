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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.31 2004/10/21 00:27:32 pavlin Exp $"


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
      _ifc_get_dummy(*this),
      _ifc_get_ioctl(*this),
      _ifc_get_sysctl(*this),
      _ifc_get_getifaddrs(*this),
      _ifc_get_proc_linux(*this),
      _ifc_get_netlink(*this),
      _ifc_get_click(*this),
      _ifc_set_dummy(*this),
      _ifc_set_ioctl(*this),
      _ifc_set_netlink(*this),
      _ifc_set_click(*this),
      _ifc_observer_dummy(*this),
      _ifc_observer_rtsock(*this),
      _ifc_observer_netlink(*this),
      _have_ipv4(false),
      _have_ipv6(false),
      _is_dummy(false),
      _is_running(false)
{
    //
    // Check that all necessary mechanisms to interact with the
    // underlying system are in place.
    //
    if (_ifc_gets.empty()) {
	XLOG_FATAL("No mechanism to get the network interface information "
		   "from the underlying system");
    }
    if (_ifc_sets.empty()) {
	XLOG_FATAL("No mechanism to set the network interface information "
		   "into the underlying system");
    }
    if (_ifc_observers.empty()) {
	XLOG_FATAL("No mechanism to observe the network interface information "
		   "from the underlying system");
    }

    //
    // Test if the system supports IPv4 and IPv6 respectively
    //
    _have_ipv4 = test_have_ipv4();
    _have_ipv6 = test_have_ipv6();
}

int
IfConfig::register_ifc_get_primary(IfConfigGet *ifc_get)
{
    _ifc_gets.clear();
    if (ifc_get != NULL) {
	_ifc_gets.push_back(ifc_get);
	ifc_get->set_primary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifc_set_primary(IfConfigSet *ifc_set)
{
    _ifc_sets.clear();
    if (ifc_set != NULL) {
	_ifc_sets.push_back(ifc_set);
	ifc_set->set_primary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifc_observer_primary(IfConfigObserver *ifc_observer)
{
    _ifc_observers.clear();
    if (ifc_observer != NULL) {
	_ifc_observers.push_back(ifc_observer);
	ifc_observer->set_primary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifc_get_secondary(IfConfigGet *ifc_get)
{
    if (ifc_get != NULL) {
	_ifc_gets.push_back(ifc_get);
	ifc_get->set_secondary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifc_set_secondary(IfConfigSet *ifc_set)
{
    if (ifc_set != NULL) {
	_ifc_sets.push_back(ifc_set);
	ifc_set->set_secondary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifc_observer_secondary(IfConfigObserver *ifc_observer)
{
    if (ifc_observer != NULL) {
	_ifc_observers.push_back(ifc_observer);
	ifc_observer->set_secondary();
    }

    return (XORP_OK);
}

int
IfConfig::set_dummy()
{
    register_ifc_get_primary(&_ifc_get_dummy);
    register_ifc_set_primary(&_ifc_set_dummy);
    register_ifc_observer_primary(&_ifc_observer_dummy);

    //
    // XXX: if we are dummy FEA, then we always have IPv4 and IPv6
    //
    _have_ipv4 = true;
    _have_ipv6 = true;

    _is_dummy = true;

    return (XORP_OK);
}

int
IfConfig::start()
{
    list<IfConfigGet*>::iterator ifc_get_iter;
    list<IfConfigSet*>::iterator ifc_set_iter;
    list<IfConfigObserver*>::iterator ifc_observer_iter;

    if (_is_running)
	return (XORP_OK);

    for (ifc_get_iter = _ifc_gets.begin();
	 ifc_get_iter != _ifc_gets.end();
	 ++ifc_get_iter) {
	IfConfigGet* ifc_get = *ifc_get_iter;
	if (ifc_get->start() < 0)
	    return (XORP_ERROR);
    }

    for (ifc_set_iter = _ifc_sets.begin();
	 ifc_set_iter != _ifc_sets.end();
	 ++ifc_set_iter) {
	IfConfigSet* ifc_set = *ifc_set_iter;
	if (ifc_set->start() < 0)
	    return (XORP_ERROR);
    }

    for (ifc_observer_iter = _ifc_observers.begin();
	 ifc_observer_iter != _ifc_observers.end();
	 ++ifc_observer_iter) {
	IfConfigObserver* ifc_observer = *ifc_observer_iter;
	if (ifc_observer->start() < 0)
	    return (XORP_ERROR);
    }

    _live_config = pull_config();
    _live_config.finalize_state();

    debug_msg("Start configuration read: %s\n", _live_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");

    _is_running = true;

    return (XORP_OK);
}

int
IfConfig::stop()
{
    list<IfConfigGet*>::iterator ifc_get_iter;
    list<IfConfigSet*>::iterator ifc_set_iter;
    list<IfConfigObserver*>::iterator ifc_observer_iter;
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    for (ifc_observer_iter = _ifc_observers.begin();
	 ifc_observer_iter != _ifc_observers.end();
	 ++ifc_observer_iter) {
	IfConfigObserver* ifc_observer = *ifc_observer_iter;
	if (ifc_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ifc_set_iter = _ifc_sets.begin();
	 ifc_set_iter != _ifc_sets.end();
	 ++ifc_set_iter) {
	IfConfigSet* ifc_set = *ifc_set_iter;
	if (ifc_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ifc_get_iter = _ifc_gets.begin();
	 ifc_get_iter != _ifc_gets.end();
	 ++ifc_get_iter) {
	IfConfigGet* ifc_get = *ifc_get_iter;
	if (ifc_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    _is_running = false;

    return (ret_value);
}

/**
 * Test if the underlying system supports IPv4.
 * 
 * @return true if the underlying system supports IPv4, otherwise false.
 */
bool
IfConfig::test_have_ipv4() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
	return true;

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
	return (false);
    
    close(s);
    return (true);
}

/**
 * Test if the underlying system supports IPv6.
 * 
 * @return true if the underlying system supports IPv6, otherwise false.
 */
bool
IfConfig::test_have_ipv6() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
	return true;

#ifndef HAVE_IPV6
    return (false);
#else
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
	return (false);
    
    close(s);
    return (true);
#endif // HAVE_IPV6
}

bool
IfConfig::push_config(const IfTree& config)
{
    list<IfConfigSet*>::iterator ifc_set_iter;

    if (_ifc_sets.empty())
	return false;

    //
    // XXX: explicitly pull the current config so we can align
    // the new config with the current config.
    //
    pull_config();

    for (ifc_set_iter = _ifc_sets.begin();
	 ifc_set_iter != _ifc_sets.end();
	 ++ifc_set_iter) {
	IfConfigSet* ifc_set = *ifc_set_iter;
	if (ifc_set->push_config(config) != true)
	    return false;
    }

    return true;
}

const IfTree&
IfConfig::pull_config()
{
    list<IfConfigGet*>::iterator ifc_get_iter;

    // Clear the old state
    _pulled_config.clear();

    for (ifc_get_iter = _ifc_gets.begin();
	 ifc_get_iter != _ifc_gets.end();
	 ++ifc_get_iter) {
	IfConfigGet* ifc_get = *ifc_get_iter;
	ifc_get->pull_config(_pulled_config);
    }

    return _pulled_config;
}

bool
IfConfig::report_update(const IfTreeInterface& fi,
			bool is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u)) {
	_ur.interface_update(fi.ifname(), u, is_system_interfaces_reportee);
	return true;
    }

    return false;
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			bool  is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fv.state(), u)) {
	_ur.vif_update(fi.ifname(), fv.vifname(), u,
		       is_system_interfaces_reportee);
	return true;
    }

    return false;
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr4&	fa,
			bool  is_system_interfaces_reportee)

{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u)) {
	_ur.vifaddr4_update(fi.ifname(), fv.vifname(), fa.addr(), u,
			    is_system_interfaces_reportee);
	return true;
    }

    return false;
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr6&	fa,
			bool is_system_interfaces_reportee)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u)) {
	_ur.vifaddr6_update(fi.ifname(), fv.ifname(), fa.addr(), u,
			    is_system_interfaces_reportee);
	return true;
    }

    return false;
}

void
IfConfig::report_updates_completed(bool is_system_interfaces_reportee)
{
    _ur.updates_completed(is_system_interfaces_reportee);
}

void
IfConfig::report_updates(const IfTree& it, bool is_system_interfaces_reportee)
{
    bool updated = false;

    //
    // Walk config looking for changes to report
    //
    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {

	const IfTreeInterface& interface = ii->second;
	updated |= report_update(interface, is_system_interfaces_reportee);

	IfTreeInterface::VifMap::const_iterator vi;
	for (vi = interface.vifs().begin();
	     vi != interface.vifs().end(); ++vi) {

	    const IfTreeVif& vif = vi->second;
	    updated |= report_update(interface, vif,
				     is_system_interfaces_reportee);

	    for (IfTreeVif::V4Map::const_iterator ai = vif.v4addrs().begin();
		 ai != vif.v4addrs().end(); ai++) {
		const IfTreeAddr4& addr = ai->second;
		updated |= report_update(interface, vif, addr,
					 is_system_interfaces_reportee);
	    }

	    for (IfTreeVif::V6Map::const_iterator ai = vif.v6addrs().begin();
		 ai != vif.v6addrs().end(); ai++) {
		const IfTreeAddr6& addr = ai->second;
		updated |= report_update(interface, vif, addr,
					 is_system_interfaces_reportee);
	    }
	}
    }

    if (updated) {
	// End if updates
	report_updates_completed(is_system_interfaces_reportee);
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
IfConfig::find_ifname(uint32_t ifindex) const
{
    IfIndex2NameMap::const_iterator i = _ifnames.find(ifindex);

    if (_ifnames.end() == i)
	return NULL;

    return i->second.c_str();
}

uint32_t
IfConfig::find_ifindex(const string& ifname) const
{
    IfIndex2NameMap::const_iterator i;

    for (i = _ifnames.begin(); i != _ifnames.end(); ++i) {
	if (i->second == ifname)
	    return i->first;
    }

    return 0;
}

const char *
IfConfig::get_insert_ifname(uint32_t ifindex)
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
IfConfig::get_insert_ifindex(const string& ifname)
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

void
IfConfigUpdateReplicator::updates_completed(bool	system)
{
    list<IfConfigUpdateReporterBase*>::iterator i = _reporters.begin();
    while (i != _reporters.end()) {
	IfConfigUpdateReporterBase*& r = *i;
	r->updates_completed(system);
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
