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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.3 2003/05/02 07:50:45 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

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
      _ifc_get_ioctl(*this),
      _ifc_get_sysctl(*this),
      _ifc_get_getifaddrs(*this),
      _ifc_set_ioctl(*this),
      _ifc_observer_rtsock(*this)
{
    pull_config(_live_config);
    _live_config.finalize_state();
    
    debug_msg("Start configuration read: %s\n", _live_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");
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

int
IfConfig::push_config(const IfTree& config)
{
    if (_ifc_set == NULL)
	return XORP_ERROR;
    return (_ifc_set->push_config(config));
}

int
IfConfig::pull_config(IfTree& config)
{
    if (_ifc_get == NULL)
	return XORP_ERROR;
    return (_ifc_get->pull_config(config));
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


void
IfConfig::map_ifindex(uint32_t index, const string& name)
{
    _ifnames[index] = name;
}
	
void
IfConfig::unmap_ifindex(uint32_t index)
{
    IfIndex2NameMap::iterator i = _ifnames.find(index);
    if (_ifnames.end() != i)
	_ifnames.erase(i);
}

const char *
IfConfig::get_ifname(uint32_t index)
{
    IfIndex2NameMap::const_iterator i = _ifnames.find(index);
    
    if (_ifnames.end() == i) {
	char name_buf[IF_NAMESIZE];
	const char* ifname = if_indextoname(index, name_buf);
	
	if (ifname)
	    map_ifindex(index, ifname);
	
	return ifname;
    }
    return i->second.c_str();
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

// -------------------------------------------------------------------------
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

