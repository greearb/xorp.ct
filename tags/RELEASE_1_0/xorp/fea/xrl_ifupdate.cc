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

#ident "$XORP: xorp/fea/xrl_ifupdate.cc,v 1.7 2004/04/10 07:58:32 pavlin Exp $"

#include "config.h"
#include "fea_module.h"
#include "libxorp/xlog.h"

#include "xrl_ifupdate.hh"
#include "xrl/interfaces/fea_ifmgr_client_xif.hh"

/******************************************************************************
 *
 * This implementation has the bug/feature that it simply signals a change
 * has occurred.  The receiver of that information then has to query what
 * the delta is.  This means an extra rtt plus processing in propapating
 * a piece of information that is time sensitive.  This design is therefore
 * not ideal, and should probably be re-evaluated when the FEA has settled
 * down.
 *
 *****************************************************************************/

/*
 * These definitions must match enumerations in:
 * 	${XORP}/xrl/fea_ifmgr_clientspy.xif
 */
enum XrlIfUpdate {
    CREATED = 1,
    DELETED = 2,
    CHANGED = 3
};

/*
 * Helper function to map Fea's concept of an event into Xrl enumerated value
 */
static XrlIfUpdate
xrl_update(IfConfigUpdateReporterBase::Update u)
{
    switch (u) {
    case IfConfigUpdateReporterBase::CREATED:
	return CREATED;
    case IfConfigUpdateReporterBase::DELETED:
	return DELETED;
    case IfConfigUpdateReporterBase::CHANGED:
	return CHANGED;
    }
    return CHANGED; /* Fix for compiler warning */
}

/* ------------------------------------------------------------------------- */
/* XrlIfConfigUpdateReporter */

XrlIfConfigUpdateReporter::XrlIfConfigUpdateReporter(XrlRouter& r) 
    : _rtr(r), _in_flight(0)
{}

bool
XrlIfConfigUpdateReporter::add_reportee(const string& tgt)
{
    if (find(_configured_interfaces_tgts.begin(),
	     _configured_interfaces_tgts.end(), tgt)
	!= _configured_interfaces_tgts.end())
	return false;
    _configured_interfaces_tgts.push_back(tgt);
    return true;
}

bool
XrlIfConfigUpdateReporter::add_system_interfaces_reportee(const string& tgt)
{
    if (find(_system_interfaces_tgts.begin(),
	     _system_interfaces_tgts.end(), tgt)
	!= _system_interfaces_tgts.end())
	return false;
    _system_interfaces_tgts.push_back(tgt);
    return true;
}

bool
XrlIfConfigUpdateReporter::has_reportee(const string& tgt) const
{
    return (find(_configured_interfaces_tgts.begin(),
		 _configured_interfaces_tgts.end(), tgt)
	    != _configured_interfaces_tgts.end());
}

bool
XrlIfConfigUpdateReporter::has_system_interfaces_reportee(const string& tgt) const
{
    return (find(_system_interfaces_tgts.begin(),
		 _system_interfaces_tgts.end(), tgt)
	    != _system_interfaces_tgts.end());
}

bool
XrlIfConfigUpdateReporter::remove_reportee(const string& tgt)
{
    TgtList::iterator ti = find(_configured_interfaces_tgts.begin(),
				_configured_interfaces_tgts.end(), tgt);
    if (ti == _configured_interfaces_tgts.end())
	return false;
    _configured_interfaces_tgts.erase(ti);
    return true;
}

bool
XrlIfConfigUpdateReporter::remove_system_interfaces_reportee(const string& tgt)
{
    TgtList::iterator ti = find(_system_interfaces_tgts.begin(),
				_system_interfaces_tgts.end(), tgt);
    if (ti == _system_interfaces_tgts.end())
	return false;
    _system_interfaces_tgts.erase(ti);
    return true;
}

void
XrlIfConfigUpdateReporter::interface_update(const string& ifname,
					    const Update& u,
					    bool  is_system_interfaces_reportee)
{
    XrlFeaIfmgrClientV0p1Client c(&_rtr);
    TgtList *tgts = NULL;
    
    if (is_system_interfaces_reportee)
	tgts = &_system_interfaces_tgts;
    else
	tgts = &_configured_interfaces_tgts;
    
    for (TgtList::const_iterator ti = tgts->begin(); ti != tgts->end(); ++ti) {
	c.send_interface_update(ti->c_str(), ifname, xrl_update(u),
	    callback(this, &XrlIfConfigUpdateReporter::xrl_sent, *ti));
	_in_flight++;
    }
}

void
XrlIfConfigUpdateReporter::vif_update(const string& ifname,
				      const string& vifname,
				      const Update& u,
				      bool  is_system_interfaces_reportee)
{
    XrlFeaIfmgrClientV0p1Client c(&_rtr);
    TgtList *tgts = NULL;
    
    if (is_system_interfaces_reportee)
	tgts = &_system_interfaces_tgts;
    else
	tgts = &_configured_interfaces_tgts;

    for (TgtList::const_iterator ti = tgts->begin(); ti != tgts->end(); ++ti) {
	c.send_vif_update(ti->c_str(), ifname, vifname, xrl_update(u),
	    callback(this, &XrlIfConfigUpdateReporter::xrl_sent, *ti));
	_in_flight++;
    }
}

void
XrlIfConfigUpdateReporter::vifaddr4_update(const string& ifname,
					   const string& vifname,
					   const IPv4&	 ip,
					   const Update& u,
					   bool  is_system_interfaces_reportee)
{
    XrlFeaIfmgrClientV0p1Client c(&_rtr);
    TgtList *tgts = NULL;
    
    if (is_system_interfaces_reportee)
	tgts = &_system_interfaces_tgts;
    else
	tgts = &_configured_interfaces_tgts;

    for (TgtList::const_iterator ti = tgts->begin(); ti != tgts->end(); ++ti) {
	c.send_vifaddr4_update(ti->c_str(), ifname, vifname, ip, xrl_update(u),
	    callback(this, &XrlIfConfigUpdateReporter::xrl_sent, *ti));
	_in_flight++;
    }
}

void
XrlIfConfigUpdateReporter::vifaddr6_update(const string& ifname,
					   const string& vifname,
					   const IPv6&	 ip,
					   const Update& u,
					   bool  is_system_interfaces_reportee)
{
    XrlFeaIfmgrClientV0p1Client c(&_rtr);
    TgtList *tgts = NULL;
    
    if (is_system_interfaces_reportee)
	tgts = &_system_interfaces_tgts;
    else
	tgts = &_configured_interfaces_tgts;

    for (TgtList::const_iterator ti = tgts->begin(); ti != tgts->end(); ++ti) {
	c.send_vifaddr6_update(ti->c_str(), ifname, vifname, ip, xrl_update(u),
	    callback(this, &XrlIfConfigUpdateReporter::xrl_sent, *ti));
	_in_flight++;
    }
}

void
XrlIfConfigUpdateReporter::updates_completed(bool  is_system_interfaces_reportee)
{
    XrlFeaIfmgrClientV0p1Client c(&_rtr);
    TgtList *tgts = NULL;
    
    if (is_system_interfaces_reportee)
	tgts = &_system_interfaces_tgts;
    else
	tgts = &_configured_interfaces_tgts;

    for (TgtList::const_iterator ti = tgts->begin(); ti != tgts->end(); ++ti) {
	c.send_updates_completed(ti->c_str(),
	    callback(this, &XrlIfConfigUpdateReporter::xrl_sent, *ti));
	_in_flight++;
    }
}

void
XrlIfConfigUpdateReporter::xrl_sent(const XrlError& e, const string tgt)
{
    _in_flight--;
    if (e != XrlError::OKAY()) {
	//
	// On an error we should think about removing target or at least
	// putting it on probation to be removed.
	//
	XLOG_ERROR("Error sending update to %s (%s), continuing.",
		   tgt.c_str(), e.str().c_str());
    }
}
