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

#ident "$XORP: xorp/fea/xrl_firewall.cc,v 1.1 2004/09/08 08:29:51 bms Exp $"

#include "fea_module.h"

#include "config.h"
#include "libxorp/xorp.h"

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xuid.hh"
#include "libcomm/comm_api.h"

#include "firewall.hh"
#include "xrl_firewall.hh"

static const char* NOT_IMPLEMENTED_MSG = "Not yet implemented.";
static const char* NOT_READY_MSG = "Underlying provider is NULL.";

// ----------------------------------------------------------------------------
// XrlFirewallTarget concrete instance constructor/destructor

XrlFirewallTarget::XrlFirewallTarget(XrlCmdMap* cmds, FirewallManager& fw)
    : XrlFirewallTargetBase(cmds), _fw(fw)
{
}

XrlFirewallTarget::~XrlFirewallTarget()
{
}

// ----------------------------------------------------------------------------
// common/0.1 interface implementation

XrlCmdError
XrlFirewallTarget::common_0_1_get_target_name(string& name)
{
    name = this->name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFirewallTarget::common_0_1_get_version(string& version)
{
    version = this->version();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFirewallTarget::common_0_1_get_status(uint32_t& status,
       string& reason)
{
    // TODO: Make this XRL more informative about the state of the
    // firewall subsystem.
    status = PROC_READY;
    reason = "";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFirewallTarget::common_0_1_shutdown()
{
    // TODO: Make this XRL tear down XORP-managed firewall rules.
    //_fw->destroy_all_rules();
    //_fw->destroy_provider();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// firewall/0.1 interface implementation

/**
 *  Function that needs to be implemented to:
 *
 *  Get global IP filter enable status.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_fw_enabled(
	// Output values,
	bool&	enabled)
{

    if (NULL != _fw._fwp) {
	enabled = _fw._fwp->get_enabled();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Set global IP filter enable status.
 *
 *  @param enabled Enable or disable IP filtering.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_set_fw_enabled(
	// Input values,
	const bool&	enabled)
{

    if (NULL != _fw._fwp) {
	_fw._fwp->set_enabled(enabled);
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get global IP filter default-to-drop status.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_fw_default_drop(
	// Output values,
	bool&	drop)
{
    UNUSED(drop);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Set global IP filter default-to-drop status.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_set_fw_default_drop(
	// Input values,
	const bool&	drop)
{
    UNUSED(drop);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the underlying IP filter provider type in use.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_fw_provider(
	// Output values,
	string&	provider)
{

    if (NULL != _fw._fwp) {
	provider = _fw._fwp->get_provider_name();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Set the underlying IP filter provider type in use.
 *
 *  @param provider Name of an IP firewall provider to use on systems which
 *  have multiple IP filtering providers.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_set_fw_provider(
	// Input values,
	const string&	provider)
{

#ifdef notyet
    if (NULL != _fw._fwp) {
	if (_fw._fwp->set_new_provider(provider) == true)
	    return XrlCmdError::OKAY();
	else
	    return XrlCmdError::COMMAND_FAILED(
"Failed to instantiate new provider.");
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
#else
    UNUSED(provider);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
#endif
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the underlying IP filter provider version in use.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_fw_version(
	// Output values,
	string&	version)
{

    if (NULL != _fw._fwp) {
	version = _fw._fwp->get_provider_version();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the number of IPv4 firewall rules installed by XORP.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_num_xorp_rules4(
	// Output values,
	uint32_t&	nrules)
{

    if (NULL != _fw._fwp) {
	nrules = _fw._fwp->get_num_xorp_rules4();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the number of IPv4 firewall rules actually visible to the
 *  underlying provider in the FEA.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_num_provider_rules4(
	// Output values,
	uint32_t&	nrules)
{

    if (NULL != _fw._fwp) {
	nrules = _fw._fwp->get_num_system_rules4();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the number of IPv6 firewall rules installed by XORP.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_num_xorp_rules6(
	// Output values,
	uint32_t&	nrules)
{

    if (NULL != _fw._fwp) {
	nrules = _fw._fwp->get_num_xorp_rules6();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

/**
 *  Function that needs to be implemented to:
 *
 *  Get the number of IPv6 firewall rules actually visible to the
 *  underlying provider in the FEA.
 */
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_num_provider_rules6(
	// Output values,
	uint32_t&	nrules)
{

    if (NULL != _fw._fwp) {
	nrules = _fw._fwp->get_num_system_rules6();
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(NOT_READY_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_add_filter4(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action)
{
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    UNUSED(action);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_add_filter6(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action)
{
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    UNUSED(action);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_delete_filter4(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport)
{
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_delete_filter6(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport)
{
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_filter_list_start4(
	// Output values,
	uint32_t&	token,
	bool&	more)
{
    UNUSED(token);
    UNUSED(more);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_filter_list_next4(
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
	string&	action)
{
    UNUSED(token);
    UNUSED(more);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    UNUSED(action);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_filter_list_start6(
	// Output values,
	uint32_t&	token,
	bool&	more)
{
    UNUSED(token);
    UNUSED(more);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}

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
XrlCmdError
XrlFirewallTarget::firewall_0_1_get_filter_list_next6(
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
	string&	action)
{
    UNUSED(token);
    UNUSED(more);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(proto);
    UNUSED(sport);
    UNUSED(dport);
    UNUSED(action);
    return XrlCmdError::COMMAND_FAILED(NOT_IMPLEMENTED_MSG);
}
