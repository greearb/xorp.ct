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

// $XORP: xorp/fea/xrl_ifupdate.hh,v 1.10 2004/04/10 07:58:32 pavlin Exp $

#ifndef __FEA_XRL_IFUPDATE_HH__
#define __FEA_XRL_IFUPDATE_HH__

#include "libxipc/xrl_router.hh"
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
     * event notifications about configured interfaces.
     *
     * @param xrl_target the xrl target to be added.
     * @return true on success, false if target already listed.
     */
    bool add_reportee(const string& xrl_target);

    /**
     * Test if an xrl target is on the list of those that will receive
     * interface event notifications about configured interfaces.
     *
     * @param xrl_target the xrl target to be tested.
     * @return true if the xrl target is on the list, otherwise false.
     */
    bool has_reportee(const string& xrl_target) const;

    /**
     * Remove an xrl target from the list of those that receive interface
     * event notifications about configured interfaces.
     *
     * @param xrl_target the xrl target to be removed.
     * @return true on success, false if target is not listed.
     */
    bool remove_reportee(const string& xrl_target);

    /**
     * Add an xrl target to list of those that will receive interface
     * event notifications about all system interfaces.
     *
     * @param xrl_target the xrl target to be added.
     * @return true on success, false if target already listed.
     */
    bool add_system_interfaces_reportee(const string& xrl_target);

    /**
     * Test if an xrl target is on the list of those that will receive
     * interface event notifications about all system interfaces.
     *
     * @param xrl_target the xrl target to be tested.
     * @return true if xrl target is on the list, otherwise false.
     */
    bool has_system_interfaces_reportee(const string& xrl_target) const;

    /**
     * Remove an xrl target from the list of those that receive interface
     * event notifications about all system interfaces.
     *
     * @param xrl_target the xrl target to be removed.
     * @return true on success, false if target is not listed.
     */
    bool remove_system_interfaces_reportee(const string& xrl_target);

    /**
     * Send announcement of interface update.
     *
     * @param ifname the name of the interface updated.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     * @param is_system_interfaces_reportee true if the update is for reportee
     * that expect event notifications about all system interfaces.
     */
    void interface_update(const string& ifname,
			  const Update& u,
			  bool		is_system_interfaces_reportee);

    /**
     * Send announcement of a vif update.
     *
     * @param ifname the name of the interface associated with vif.
     * @param vifname the name of the vif updated.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     * @param is_system_interfaces_reportee true if the update is for reportee
     * that expect event notifications about all system interfaces.
     */
    void vif_update(const string&	ifname,
		    const string&	vifname,
		    const Update&	u,
		    bool		is_system_interfaces_reportee);

    /**
     * Send announcement of a vif address update.
     *
     * @param ifname the name of the interface associated with vif address.
     * @param vifname the name of the vif associated with the address.
     * @param addr the updated address.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     * @param is_system_interfaces_reportee true if the update is for reportee
     * that expect event notifications about all system interfaces.
     */
    void vifaddr4_update(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr,
			 const Update&	u,
			 bool		is_system_interfaces_reportee);

    /**
     * Send announcement of a vif address update.
     *
     * @param ifname the name of the interface associated with vif address.
     * @param vifname the name of the vif associated with the address.
     * @param addr the updated address.
     * @param u the update that occured
     * @see IfConfigUpdateReporterBase#Update
     * @param is_system_interfaces_reportee true if the update is for reportee
     * that expect event notifications about all system interfaces.
     */
    void vifaddr6_update(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr,
			 const Update&	u,
			 bool		is_system_interfaces_reportee);

    /**
     * Send announcement that updates have been completed.
     *
     * @param is_system_interfaces_reportee true if the update is for reportee
     * that expect event notifications about all system interfaces.
     */
    void updates_completed(bool		is_system_interfaces_reportee);

    /**
     * @return true if we have config changes in flight
     */
    bool busy() const {return (_in_flight > 0);}

protected:
    void xrl_sent(const XrlError&e, const string tgt);

    XrlIfConfigUpdateReporter(const XrlIfConfigUpdateReporter&);
    XrlIfConfigUpdateReporter& operator=(const XrlIfConfigUpdateReporter&);

protected:
    XrlRouter&	_rtr;
    TgtList	_configured_interfaces_tgts;	// Targets for configured vifs
    TgtList	_system_interfaces_tgts;	// Targets for all system vifs
    uint32_t    _in_flight;
};

#endif // __FEA_XRL_IFUPDATE_HH__
