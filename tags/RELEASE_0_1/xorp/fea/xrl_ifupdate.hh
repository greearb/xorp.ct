// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_ifupdate.hh,v 1.6 2002/12/09 18:29:00 hodson Exp $

#ifndef __FEA_XRL_IFUPDATE_HH__
#define __FEA_XRL_IFUPDATE_HH__

#include "libxipc/xrlrouter.hh"
#include "ifconfig.hh"

/**
 * @short Class for exporting configuration events within FEA to other
 *        processes via XRL's.
 */
class XrlIfConfigUpdateReporter : public IfConfigUpdateReporterBase
{
public:
    typedef IfConfigUpdateReporterBase::Update Update;
    typedef list<string> TgtList;

    XrlIfConfigUpdateReporter(XrlRouter& rtr);

    /**
     * Add an xrl target to list of those that will receive interface
     * event notifications.
     *
     * @param xrl_target the xrl target to be added.
     * @return true on success, false if target already listed.
     */
    bool add_reportee(const string& xrl_target);

    /**
     * Remove an xrl target from the list of those that receive interface
     * event notifications.
     *
     * @param xrl_target the xrl target to be removed.
     * @return true on success, false if target is not listed.
     */
    bool remove_reportee(const string& xrl_target);

    /**
     * Send announcment of interface update.
     *
     * @param ifname the name of the interface updated.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     */
    void interface_update(const string& ifname,
			  const Update& u);

    /**
     * Send announcment of a vif update.
     *
     * @param ifname the name of the interface associated with vif.
     * @param vifname the name of the vif updated.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     */
    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& u);

    /**
     * Send announcment of a vif address update.
     *
     * @param ifname the name of the interface associated with vif address.
     * @param vifname the name of the vif associated with the address.
     * @param addr the updated address.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     */
    void vifaddr4_update(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr,
			 const Update&	u);

    /**
     * Send announcment of a vif address update.
     *
     * @param ifname the name of the interface associated with vif address.
     * @param vifname the name of the vif associated with the address.
     * @param addr the updated address.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     */
    void vifaddr6_update(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr,
			 const Update&	u);

protected:
    void xrl_sent(const XrlError&e, const string tgt);

    XrlIfConfigUpdateReporter(const XrlIfConfigUpdateReporter&);
    XrlIfConfigUpdateReporter& operator=(const XrlIfConfigUpdateReporter&);

protected:
    XrlRouter&	_rtr;
    TgtList	_tgts;
};

#endif // __FEA_XRL_IFUPDATE_HH__
