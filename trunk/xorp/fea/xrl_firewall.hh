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

// $XORP$

#ifndef __FEA_XRL_FIREWALL_HH__
#define __FEA_XRL_FIREWALL_HH__

#include "libxorp/xlog.h"
#include "libxipc/xrl_cmd_map.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"

#include "firewall.hh"
#include "xrl/targets/firewall_base.hh"

/**
 * Helper class for helping with Firewall configuration transactions
 * via an Xrl interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the FirewallManager
 * class.
 */
class XrlFirewallTarget : public XrlFirewallTargetBase {
protected:
    FirewallManager&	_fw;	// Associated FirewallManager container

public:
    /**
     * Constructor.
     *
     * @param cmds an XrlCmdMap that the commands associated with the target
     *		   should be added to.  This is typically the XrlRouter
     *		   associated with the target.
     */
    XrlFirewallTarget(XrlCmdMap* cmds, FirewallManager& fw);

    /**
     * Destructor.
     *
     * Dissociates instance commands from command map.
     */
     virtual ~XrlFirewallTarget();

    /**
     * Set command map.
     *
     * @param cmds pointer to command map to associate commands with.  This
     * argument is typically a pointer to the XrlRouter associated with the
     * target.
     *
     * @return true on success, false if cmds is null or a command map has
     * already been supplied.
     */
    bool set_command_map(XrlCmdMap* cmds);

    /**
     * Get Xrl instance name associated with command map.
     */
    inline const string& name() const { return _cmds->name(); }

    /**
     * Get version string of instance.
     */
    inline const char* version() const { return "firewall/0.1"; }

protected:

    /**
     *  Function that needs to be implemented to:
     *
     *  Get name of Xrl Target
     */
     XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get version string from Xrl Target
     */
     XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get status of Xrl Target
     */
     XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Function that needs to be implemented to:
     *
     *  Request clean shutdown of Xrl Target
     */
     XrlCmdError common_0_1_shutdown();

    /**
     *  Function that needs to be implemented to:
     *
     *  Get global IP filter enable status.
     */
     XrlCmdError firewall_0_1_get_fw_enabled(
	// Output values,
	bool&	enabled);

    /**
     *  Function that needs to be implemented to:
     *
     *  Set global IP filter enable status.
     *
     *  @param enabled Enable or disable IP filtering.
     */
     XrlCmdError firewall_0_1_set_fw_enabled(
	// Input values,
	const bool&	enabled);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get global IP filter default-to-drop status.
     */
     XrlCmdError firewall_0_1_get_fw_default_drop(
	// Output values,
	bool&	drop);

    /**
     *  Function that needs to be implemented to:
     *
     *  Set global IP filter default-to-drop status.
     */
     XrlCmdError firewall_0_1_set_fw_default_drop(
	// Input values,
	const bool&	drop);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the underlying IP filter provider type in use.
     */
     XrlCmdError firewall_0_1_get_fw_provider(
	// Output values,
	string&	provider);

    /**
     *  Function that needs to be implemented to:
     *
     *  Set the underlying IP filter provider type in use.
     *
     *  @param provider Name of an IP firewall provider to use on systems which
     *  have multiple IP filtering providers.
     */
     XrlCmdError firewall_0_1_set_fw_provider(
	// Input values,
	const string&	provider);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the underlying IP filter provider version in use.
     */
     XrlCmdError firewall_0_1_get_fw_version(
	// Output values,
	string&	version);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the number of IPv4 firewall rules installed by XORP.
     */
     XrlCmdError firewall_0_1_get_num_xorp_rules4(
	// Output values,
	uint32_t&	nrules);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the number of IPv4 firewall rules actually visible to the
     *  underlying provider in the FEA.
     */
     XrlCmdError firewall_0_1_get_num_provider_rules4(
	// Output values,
	uint32_t&	nrules);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the number of IPv6 firewall rules installed by XORP.
     */
     XrlCmdError firewall_0_1_get_num_xorp_rules6(
	// Output values,
	uint32_t&	nrules);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the number of IPv6 firewall rules actually visible to the
     *  underlying provider in the FEA.
     */
     XrlCmdError firewall_0_1_get_num_provider_rules6(
	// Output values,
	uint32_t&	nrules);

    /**
     *  Function that needs to be implemented to:
     *
     *  Add an IPv4 family filter rule.
     *
     *  @param ifname Name of the interface where this filter is to be applied.
     *
     *  @param vifname Name of the vif where this filter is to be applied.
     *
     *  @param src Source IPv4 address with network prefix.
     *
     *  @param dst Destination IPv4 address with network prefix.
     *
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param action Action to take when this filter is matched.
     */
     XrlCmdError firewall_0_1_add_filter4(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *
     *  Add an IPv6 family filter rule.
     *
     *  @param ifname Name of the interface where this filter is to be applied.
     *
     *  @param vifname Name of the vif where this filter is to be applied.
     *
     *  @param src Source IPv6 address with network prefix.
     *
     *  @param dst Destination IPv6 address with network prefix.
     *
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param action Action to take when this filter is matched.
     */
     XrlCmdError firewall_0_1_add_filter6(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *
     *  Delete an IPv4 family filter rule.
     *
     *  @param ifname Name of the interface where this filter is to be deleted.
     *
     *  @param vifname Name of the vif where this filter is to be deleted.
     *
     *  @param src Source IPv4 address with network prefix.
     *
     *  @param dst Destination IPv4 address with network prefix.
     *
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError firewall_0_1_delete_filter4(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *
     *  Delete an IPv6 family filter rule.
     *
     *  @param ifname Name of the interface where this filter is to be deleted.
     *
     *  @param vifname Name of the vif where this filter is to be deleted.
     *
     *  @param src Source IPv6 address with network prefix.
     *
     *  @param dst Destination IPv6 address with network prefix.
     *
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError firewall_0_1_delete_filter6(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the first IPv4 family filter rule configured in the system.
     *
     *  @param token returned token to be provided when calling
     *  get_filter_list_next4.
     *
     *  @param more returned to indicate whether there are more list items
     *  remaining.
     */
     XrlCmdError firewall_0_1_get_filter_list_start4(
	// Output values,
	uint32_t&	token,
	bool&	more);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the next IPv4 family filter rule configured in the system.
     *
     *  @param token token from prior call to get_filter_list_start4.
     *
     *  @param more returned to indicate whether there are more list items
     *  remaining.
     */
     XrlCmdError firewall_0_1_get_filter_list_next4(
	// Input values,
	const uint32_t&	token,
	// Output values,
	bool&	more,
	string&	ifname,
	string&	vifname,
	IPv4Net&	src,
	IPv4Net&	dst,
	uint32_t&	proto,
	uint32_t&	sport,
	uint32_t&	dport,
	string&	action);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the first IPv6 family filter rule configured in the system.
     *
     *  @param token returned token to be provided when calling
     *  get_filter_list_next6.
     *
     *  @param more returned to indicate whether there are more list items
     *  remaining.
     */
     XrlCmdError firewall_0_1_get_filter_list_start6(
	// Output values,
	uint32_t&	token,
	bool&	more);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get the next IPv6 family filter rule configured in the system.
     *
     *  @param token token from prior call to get_filter_list_start6.
     *
     *  @param more returned to indicate whether there are more list items
     *  remaining.
     */
     XrlCmdError firewall_0_1_get_filter_list_next6(
	// Input values,
	const uint32_t&	token,
	// Output values,
	bool&	more,
	string&	ifname,
	string&	vifname,
	IPv6Net&	src,
	IPv6Net&	dst,
	uint32_t&	proto,
	uint32_t&	sport,
	uint32_t&	dport,
	string&	action);

};

#endif /* __FEA_XRL_FIREWALL_HH__ */
