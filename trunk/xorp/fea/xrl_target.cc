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

#ident "$XORP: xorp/fea/xrl_target.cc,v 1.59 2004/11/29 02:56:36 pavlin Exp $"

#define PROFILE_UTILS_REQUIRED

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"

#include "net/if.h"			/* Included for IFF_ definitions */

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/profile_client_xif.hh"
#include "libxorp/profile.hh"

#include "fticonfig.hh"
#include "libfeaclient_bridge.hh"
#include "xrl_ifupdate.hh"
#include "xrl_rawsock4.hh"
//#include "xrl_rawsock6.hh"
#include "xrl_socket_server.hh"
#include "xrl_target.hh"
#include "profile_vars.hh"

XrlFeaTarget::XrlFeaTarget(EventLoop&		 	e,
			   XrlRouter&		 	r,
			   FtiConfig&  		 	ftic,
			   InterfaceManager&	 	ifmgr,
			   XrlIfConfigUpdateReporter&	xifcur,
			   Profile&			profile,
			   XrlRawSocket4Manager*	xrsm4,
//			   XrlRawSocket6Manager*	xrsm6,
			   LibFeaClientBridge*		lfcb,
			   XrlSocketServer*		xss)
    : XrlFeaTargetBase(&r), _xrl_router(r), _xftm(e, ftic, &r),
      _xifmgr(e, ifmgr), _xifcur(xifcur), _profile(profile),
      _xrsm4(xrsm4), //_xrsm6(xrsm6),
      _lfcb(lfcb), _xss(xss), _done(false),
      _have_ipv4(false), _have_ipv6(false)
{
    _have_ipv4 = _xftm.ftic().have_ipv4();
    _have_ipv6 = _xftm.ftic().have_ipv6();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_target_name(
					 // Output values,
					 string&	name)
{
    name = "fea";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_version(
				     // Output values,
				     string&   version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_status(
				    // Output values,
				    uint32_t& status,
				    string&	reason)
{
    ProcessStatus s;
    string r;
    s = _xifmgr.status(r);
    //if it's bad news, don't bother to ask any other modules.
    switch (s) {
    case PROC_FAILED:
    case PROC_SHUTDOWN:
    case PROC_DONE:
	status = s;
	reason = r;
	return XrlCmdError::OKAY();
    case PROC_NOT_READY:
	reason = r;
	break;
    case PROC_READY:
	break;
    case PROC_NULL:
	//can't be running and in this state
	abort();
    case PROC_STARTUP:
	//can't be responding to an XRL and in this state
	abort();
    }
    status = s;

    if (_done) {
	status = PROC_DONE;	// XXX: the process was shutdown
	reason = "";
    }

    if (_xifcur.busy()) {
	status = PROC_NOT_READY;
	reason = "Communicating config changes to other processes";
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_shutdown()
{
    _done = true;

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable Click FEA support.
 *
 *  @param enable if true, then enable the Click FEA support, otherwise
 *  disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_click(
    // Input values,
    const bool&	enable)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.enable_click(enable);
    ftic.enable_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Start Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_start_click()
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();
    string error_msg;

    if (ifc.start_click(error_msg) < 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (ftic.start_click(error_msg) < 0) {
	string dummy_msg;
	ifc.stop_click(error_msg);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Stop Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_stop_click()
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();
    string error_msg1, error_msg2;

    if (ftic.stop_click(error_msg1) < 0) {
	ifc.stop_click(error_msg2);
	return XrlCmdError::COMMAND_FAILED(error_msg1);
    }
    if (ifc.stop_click(error_msg1) < 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg1);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Specify the external program to generate the Click configuration.
 *
 *  @param click_config_generator_file the name of the external program to
 *  generate the Click configuration.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_click_config_generator_file(
    // Input values,
    const string&	click_config_generator_file)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_click_config_generator_file(click_config_generator_file);
    ftic.set_click_config_generator_file(click_config_generator_file);

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable kernel-level Click FEA support.
 *
 *  @param enable if true, then enable the kernel-level Click FEA support,
 *  otherwise disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_kernel_click(
    // Input values,
    const bool&	enable)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.enable_kernel_click(enable);
    ftic.enable_kernel_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable user-level Click FEA support.
 *
 *  @param enable if true, then enable the user-level Click FEA support,
 *  otherwise disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_user_click(
    // Input values,
    const bool&	enable)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.enable_user_click(enable);
    ftic.enable_user_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the user-level Click command file.
 *
 *  @param user_click_command_file the name of the user-level Click command
 *  file.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_file(
    // Input values,
    const string&	user_click_command_file)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_command_file(user_click_command_file);
    ftic.set_user_click_command_file(user_click_command_file);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the extra arguments to the user-level Click command.
 *
 *  @param user_click_command_extra_arguments the extra arguments to the
 *  user-level Click command.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_extra_arguments(
    // Input values,
    const string&	user_click_command_extra_arguments)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_command_extra_arguments(user_click_command_extra_arguments);
    ftic.set_user_click_command_extra_arguments(user_click_command_extra_arguments);

    return XrlCmdError::OKAY();
}

/**
 *  Specify whether to execute on startup the user-level Click command.
 *
 *  @param user_click_command_execute_on_startup if true, then execute the
 *  user-level Click command on startup.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_execute_on_startup(
    // Input values,
    const bool&	user_click_command_execute_on_startup)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_command_execute_on_startup(user_click_command_execute_on_startup);
    ftic.set_user_click_command_execute_on_startup(user_click_command_execute_on_startup);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the address to use for control access to the user-level
 *  Click.
 *
 *  @param user_click_control_address the address to use for
 *  control access to the user-level Click.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_control_address(
    // Input values,
    const IPv4&	user_click_control_address)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_control_address(user_click_control_address);
    ftic.set_user_click_control_address(user_click_control_address);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the socket port to use for control access to the user-level
 *  Click.
 *
 *  @param user_click_control_socket_port the socket port to use for
 *  control access to the user-level Click.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_control_socket_port(
    // Input values,
    const uint32_t&	user_click_control_socket_port)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_control_socket_port(user_click_control_socket_port);
    ftic.set_user_click_control_socket_port(user_click_control_socket_port);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the configuration file to be used by user-level Click on
 *  startup.
 *
 *  @param user_click_startup_config_file the name of the configuration
 *  file to be used by user-level Click on startup.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_startup_config_file(
    // Input values,
    const string&	user_click_startup_config_file)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    FtiConfig& ftic = _xftm.ftic();

    ifc.set_user_click_startup_config_file(user_click_startup_config_file);
    ftic.set_user_click_startup_config_file(user_click_startup_config_file);

    return XrlCmdError::OKAY();
}

/**
 *  Add a FIB client.
 *
 *  @param target_name the target name of the FIB client to add.
 */
XrlCmdError
XrlFeaTarget::fea_fib_0_1_add_fib_client4(
    // Input values,
    const string&	target_name,
    const bool&		send_updates,
    const bool&		send_resolves)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xftm.add_fib_client4(target_name, send_updates, send_resolves);
}

XrlCmdError
XrlFeaTarget::fea_fib_0_1_add_fib_client6(
    // Input values,
    const string&	target_name,
    const bool&		send_updates,
    const bool&		send_resolves)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    return _xftm.add_fib_client6(target_name, send_updates, send_resolves);
}

/**
 *  Delete a FIB client.
 *
 *  @param target_name the target name of the FIB client to delete.
 */
XrlCmdError
XrlFeaTarget::fea_fib_0_1_delete_fib_client4(
    // Input values,
    const string&	target_name)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xftm.delete_fib_client4(target_name);
}

XrlCmdError
XrlFeaTarget::fea_fib_0_1_delete_fib_client6(
    // Input values,
    const string&	target_name)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    return _xftm.delete_fib_client6(target_name);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_interface_names(
						// Output values,
						XrlAtomList&	ifnames)
{
    const IfTree& it = _xifmgr.ifconfig().pull_config();

    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second.ifname()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_names(
						       // Output values,
						       XrlAtomList&	ifnames)
{
    const IfTree& it = _xifmgr.iftree();

    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second.ifname()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_names(
					  const string& ifname,
					  // Output values,
					  XrlAtomList&	vifnames)
{
    const IfTreeInterface* fip = 0;
    XrlCmdError e = _xifmgr.pull_config_get_if(ifname, fip);

    if (e == XrlCmdError::OKAY()) {
	for (IfTreeInterface::VifMap::const_iterator vi = fip->vifs().begin();
	     vi != fip->vifs().end(); ++vi) {
	    vifnames.append(XrlAtom(vi->second.vifname()));
	}
    }

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_names(
						 const string& ifname,
						 // Output values,
						 XrlAtomList&  vifnames)
{
    const IfTreeInterface* fip = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fip);

    if (e == XrlCmdError::OKAY()) {
	for (IfTreeInterface::VifMap::const_iterator vi = fip->vifs().begin();
	     vi != fip->vifs().end(); ++vi) {
	    vifnames.append(XrlAtom(vi->second.vifname()));
	}
    }

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_flags(
    // Input values,
    const string&	ifname,
    const string&	vif,
    // Output values,
    bool&		enabled,
    bool&		broadcast,
    bool&		loopback,
    bool&		point_to_point,
    bool&		multicast)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    enabled = fv->enabled();
    broadcast = fv->broadcast();
    loopback = fv->loopback();
    point_to_point = fv->point_to_point();
    multicast = fv->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_flags(
    // Input values,
    const string&	ifname,
    const string&	vif,
    // Output values,
    bool&		enabled,
    bool&		broadcast,
    bool&		loopback,
    bool&		point_to_point,
    bool&		multicast)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    enabled = fv->enabled();
    broadcast = fv->broadcast();
    loopback = fv->loopback();
    point_to_point = fv->point_to_point();
    multicast = fv->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_pif_index(
    // Input values,
    const string&	ifname,
    const string&	vif,
    // Output values,
    uint32_t&		pif_index)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    pif_index = fv->pif_index();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_pif_index(
    // Input values,
    const string&	ifname,
    const string&	vif,
    // Output values,
    uint32_t&		pif_index)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    pif_index = fv->pif_index();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_interface_enabled(
					      // Input values,
					      const string&	ifname,
					      // Output values,
					      bool&		enabled)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.pull_config_get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	enabled = fi->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_interface_discard(
					      // Input values,
					      const string&	ifname,
					      // Output values,
					      bool&		discard)
{
    const IfTreeInterface* fi = 0;
    // XXX: We assume that the discard property exists wholly in the FEA,
    // and that it is never set by the underlying network stack; therefore
    // we never 'pull' it.
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	discard = fi->discard();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_enabled(
					      // Input values,
					      const string&	ifname,
					      // Output values,
					      bool&		enabled)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	enabled = fi->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_discard(
					      // Input values,
					      const string&	ifname,
					      // Output values,
					      bool&		discard)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	discard = fi->discard();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_mac(
				// Input values,
				const string& ifname,
				Mac&	    mac)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.pull_config_get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mac = fi->mac();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_mac(
				// Input values,
				const string& ifname,
				Mac&	    mac)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mac = fi->mac();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_mtu(
				// Input values,
				const string&	ifname,
				uint32_t&	mtu)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.pull_config_get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mtu = fi->mtu();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_mtu(
				// Input values,
				const string&	ifname,
				uint32_t&	mtu)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mtu = fi->mtu();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_enabled(
					// Input values,
					const string& ifname,
					const string& vifname,
					// Output values,
					bool&	    enabled)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vifname, fv);

    if (e == XrlCmdError::OKAY())
	enabled = fv->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_enabled(
					// Input values,
					const string& ifname,
					const string& vifname,
					// Output values,
					bool&	    enabled)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vifname, fv);

    if (e == XrlCmdError::OKAY())
	enabled = fv->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_prefix4(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv4&	addr,
				    // Output values,
				    uint32_t&		prefix_len)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix_len = fa->prefix_len();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_prefix4(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv4&	addr,
				    // Output values,
				    uint32_t&		prefix_len)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix_len = fa->prefix_len();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_broadcast4(
				       // Input values,
				       const string&	ifname,
				       const string&	vifname,
				       const IPv4&	addr,
				       // Output values,
				       IPv4&		broadcast)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    broadcast = fa->bcast();
    return _xifmgr.addr_valid(ifname, vifname, addr, "broadcast", broadcast);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_broadcast4(
				       // Input values,
				       const string&	ifname,
				       const string&	vifname,
				       const IPv4&	addr,
				       // Output values,
				       IPv4&		broadcast)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    broadcast = fa->bcast();
    return _xifmgr.addr_valid(ifname, vifname, addr, "broadcast", broadcast);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_endpoint4(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv4&	addr,
				      // Output values,
				      IPv4&		endpoint)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return _xifmgr.addr_valid(ifname, vifname, addr, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_endpoint4(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv4&	addr,
				      // Output values,
				      IPv4&		endpoint)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return _xifmgr.addr_valid(ifname, vifname, addr, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_prefix6(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv6&		addr,
				    // Output values,
				    uint32_t&		prefix_len)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix_len = fa->prefix_len();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_prefix6(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv6&		addr,
				    // Output values,
				    uint32_t&		prefix_len)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix_len = fa->prefix_len();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_endpoint6(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      // Output values,
				      IPv6&		endpoint)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return  _xifmgr.addr_valid(ifname, vifname, addr, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_endpoint6(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      // Output values,
				      IPv6&		endpoint)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return  _xifmgr.addr_valid(ifname, vifname, addr, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_addresses6(
					       // Input values,
					       const string&	ifname,
					       const string&	vif,
					       // Output values,
					       XrlAtomList&	addrs)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    // XXX This should do a pull up
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V6Map::const_iterator ai = fv->v6addrs().begin();
	 ai != fv->v6addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses6(
						      // Input values,
						      const string&	ifname,
						      const string&	vif,
						      // Output values,
						      XrlAtomList&	addrs)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V6Map::const_iterator ai = fv->v6addrs().begin();
	 ai != fv->v6addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_address_flags4(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv4&   address,
				       // Output values
				       bool& up,
				       bool& broadcast,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    up = fa->enabled();
    broadcast = fa->broadcast();
    loopback = fa->loopback();
    point_to_point = fa->point_to_point();
    multicast = fa->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_flags4(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv4&   address,
				       // Output values
				       bool& up,
				       bool& broadcast,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa;
    XrlCmdError e = _xifmgr.get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    up = fa->enabled();
    broadcast = fa->broadcast();
    loopback = fa->loopback();
    point_to_point = fa->point_to_point();
    multicast = fa->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_address_flags6(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv6&   address,
				       // Output values
				       bool& up,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    up = fa->enabled();
    loopback = fa->loopback();
    point_to_point = fa->point_to_point();
    multicast = fa->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_flags6(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv6&   address,
				       // Output values
				       bool& up,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa;
    XrlCmdError e = _xifmgr.get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    up = fa->enabled();
    loopback = fa->loopback();
    point_to_point = fa->point_to_point();
    multicast = fa->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_vif_addresses4(
					       // Input values,
					       const string&	ifname,
					       const string&	vif,
					       // Output values,
					       XrlAtomList&	addrs)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    // XXX This should do a pull up
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V4Map::const_iterator ai = fv->v4addrs().begin();
	 ai != fv->v4addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses4(
						      // Input values,
						      const string&	ifname,
						      const string&	vif,
						      // Output values,
						      XrlAtomList&	addrs)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V4Map::const_iterator ai = fv->v4addrs().begin();
	 ai != fv->v4addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_start_transaction(
					  // Output values,
					  uint32_t& tid)
{
    return _xifmgr.start_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_commit_transaction(
					   // Input values,
					   const uint32_t& tid)
{
    return _xifmgr.commit_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_abort_transaction(
					  // Input values,
					  const uint32_t& tid)
{
    return _xifmgr.abort_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_interface(
					 // Input values,
					 const uint32_t&	tid,
					 const string&		ifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddInterface(it, ifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_interface(
					 // Input values,
					 const uint32_t&	tid,
					 const string&		ifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveInterface(it, ifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_configure_interface_from_system(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfConfig& ifc = _xifmgr.ifconfig();
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new ConfigureInterfaceFromSystem(ifc, it, ifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_enabled(
					      // Input values,
					      const uint32_t&	tid,
					      const string&	ifname,
					      const bool&	enabled)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceEnabled(it, ifname, enabled));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_discard(
					      // Input values,
					      const uint32_t&	tid,
					      const string&	ifname,
					      const bool&	discard)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceDiscard(it, ifname, discard));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mac(
				// Input values,
				const uint32_t&	tid,
				const string&	ifname,
				const Mac&	mac)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceMAC(it, ifname, mac));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mtu(
				// Input values,
				const uint32_t&	tid,
				const string&	ifname,
				const uint32_t&	mtu)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceMTU(it, ifname, mtu));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_vif(
				   // Input values,
				   const uint32_t&	tid,
				   const string&	ifname,
				   const string&	vifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddInterfaceVif(it, ifname, vifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_vif(
				   // Input values,
				   const uint32_t&	tid,
				   const string&	ifname,
				   const string&	vifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveInterfaceVif(it, ifname, vifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_vif_enabled(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const bool&	enabled)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetVifEnabled(it, ifname, vifname, enabled));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address4(
					// Input values,
					const uint32_t& tid,
					const string&   ifname,
					const string&   vifname,
					const IPv4&	address)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddAddr4(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address4(
					// Input values,
					const uint32_t& tid,
					const string&   ifname,
					const string&   vifname,
					const IPv4&     address)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveAddr4(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled4(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv4&	address,
					const bool&	en)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr4Enabled(it, ifname, vifname, address, en));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_address_enabled4(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv4&	address,
					     bool&		enabled)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_enabled4(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv4&	address,
					     bool&		enabled)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix4(
				    // Input values,
				    const uint32_t&	tid,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv4&		address,
				    const uint32_t&	prefix_len)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Prefix(it, ifname, vifname, address,
					       prefix_len));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_broadcast4(
				       // Input values,
				       const uint32_t&	tid,
				       const string&	ifname,
				       const string&	vifname,
				       const IPv4&	address,
				       const IPv4&	broadcast)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Broadcast(it, ifname, vifname, address,
						  broadcast));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint4(
				      // Input values,
				      const uint32_t&	tid,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv4&	address,
				      const IPv4&	endpoint)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Endpoint(it, ifname, vifname, address,
						 endpoint));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address6(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv6&	address)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddAddr6(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address6(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv6&	address)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveAddr6(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled6(
					     // Input values,
					     const uint32_t&	tid,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv6&	address,
					     const bool&	en)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Enabled(it, ifname, vifname, address, en));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_system_address_enabled6(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv6&	address,
					     bool&		enabled)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.pull_config_get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_enabled6(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv6&	address,
					     bool&		enabled)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix6(
				    // Input values,
				    const uint32_t&	tid,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv6&	  	address,
				    const uint32_t&	prefix_len)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Prefix(it, ifname, vifname, address,
					  prefix_len));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint6(
				      // Input values,
				      const uint32_t&	tid,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv6&	address,
				      const IPv6&	endpoint)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Endpoint(it, ifname, vifname, address,
					    endpoint));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_register_client(const string& client)
{
    if (_xifcur.has_reportee(client)) {
	XLOG_WARNING("Registering again client %s", client.c_str());
	return XrlCmdError::OKAY();
    }
    if (_xifcur.add_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" cannot be registered."));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_unregister_client(const string& client)
{
    if (_xifcur.remove_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" not registered."));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_register_system_interfaces_client(const string& client)
{
    if (_xifcur.has_system_interfaces_reportee(client)) {
	XLOG_WARNING("Registering again client %s", client.c_str());
	return XrlCmdError::OKAY();
    }
    if (_xifcur.add_system_interfaces_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" cannot be registered."));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_unregister_system_interfaces_client(const string& client)
{
    if (_xifcur.remove_system_interfaces_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" not registered."));
}

XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_register_ifmgr_mirror(
	const string& clientname
	)
{
    if (_lfcb == 0) {
	return XrlCmdError::COMMAND_FAILED("Service not available.");
    }
    if (_lfcb->add_libfeaclient_mirror(clientname) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_unregister_ifmgr_mirror(
	const string& clientname
	)
{
    if (_lfcb == 0) {
	return XrlCmdError::COMMAND_FAILED("Service not available.");
    }
    if (_lfcb->remove_libfeaclient_mirror(clientname) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// FTI Related

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_dest4(
	// Input values,
	const IPv4&	dst,
	// Output values,
	IPv4Net&	netmask,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    Fte4 fte;
    if (_xftm.ftic().lookup_route_by_dest4(dst, fte) == true) {
	netmask = fte.net();
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_dest6(
	// Input values,
	const IPv6&	dst,
	// Output values,
	IPv6Net&	netmask,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    Fte6 fte;
    if (_xftm.ftic().lookup_route_by_dest6(dst, fte) == true) {
	netmask = fte.net();
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_network4(
	// Input values,
	const IPv4Net&	dst,
	// Output values,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    Fte4 fte;
    if (_xftm.ftic().lookup_route_by_network4(dst, fte)) {
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_network6(
	// Input values,
	const IPv6Net&	dst,
	// Output values,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    Fte6 fte;
    if (_xftm.ftic().lookup_route_by_network6(dst, fte)) {
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_have_ipv4(
	// Output values,
	bool&	result)
{
    result = _xftm.ftic().have_ipv4();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_have_ipv6(
	// Output values,
	bool&	result)
{
    result = _xftm.ftic().have_ipv6();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled4(
	// Output values,
	bool&	enabled)
{
    string error_msg;

    if (_xftm.ftic().unicast_forwarding_enabled4(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled6(
	// Output values,
	bool&	enabled)
{
    string error_msg;

    if (_xftm.ftic().unicast_forwarding_enabled6(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled4(
	// Input values,
	const bool&	enabled)
{
    string error_msg;

    if (_xftm.ftic().set_unicast_forwarding_enabled4(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled6(
	// Input values,
	const bool&	enabled)
{
    string error_msg;

    if (_xftm.ftic().set_unicast_forwarding_enabled6(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

//
// RIB routes redistribution transaction-based XRL interface
//

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    return _xftm.start_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    return _xftm.commit_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    return _xftm.abort_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_add_route(
    // Input values,
    const uint32_t&	tid,
    const IPv4Net&	dst,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    //
    // XXX: don't add/delete routes for directly-connected subnets,
    // because that should be managed by the underlying system.
    //
    if (protocol_origin == "connected")
	return XrlCmdError::OKAY();

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in, c_format("add %s", dst.str().c_str()));

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiAddEntry4(_xftm.ftic(), dst, nexthop, ifname, vifname, metric,
			 admin_distance)
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_delete_route(
    // Input values,
    const uint32_t&	tid,
    const IPv4Net&	network,
    const string&	cookie,
    const string&	protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    //
    // XXX: don't add/delete routes for directly-connected subnets,
    // because that should be managed by the underlying system.
    //
    if (protocol_origin == "connected")
	return XrlCmdError::OKAY();

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in,
		     c_format("delete %s", network.str().c_str()));

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiDeleteEntry4(_xftm.ftic(), network)
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.

    FtiTransactionManager::Operation op(
	new FtiDeleteAllEntries4(_xftm.ftic())
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    return _xftm.start_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    return _xftm.commit_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    return _xftm.abort_transaction(tid);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_add_route(
    // Input values,
    const uint32_t&	tid,
    const IPv6Net&	dst,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    //
    // XXX: don't add/delete routes for directly-connected subnets,
    // because that should be managed by the underlying system.
    //
    if (protocol_origin == "connected")
	return XrlCmdError::OKAY();

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in, c_format("add %s", dst.str().c_str()));

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiAddEntry6(_xftm.ftic(), dst, nexthop, ifname, vifname, metric,
			 admin_distance)
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_delete_route(
    // Input values,
    const uint32_t&	tid,
    const IPv6Net&	network,
    const string&	cookie,
    const string&	protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    //
    // XXX: don't add/delete routes for directly-connected subnets,
    // because that should be managed by the underlying system.
    //
    if (protocol_origin == "connected")
	return XrlCmdError::OKAY();

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in,
		     c_format("delete %s", network.str().c_str()));

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiDeleteEntry6(_xftm.ftic(), network)
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.

    FtiTransactionManager::Operation op(
	new FtiDeleteAllEntries6(_xftm.ftic())
	);
    return _xftm.add(tid, op);

    UNUSED(cookie);
}

// ----------------------------------------------------------------------------
// IPv4 Raw Socket related

static const char* XRL_RAW_SOCKET4_NULL = "XrlRawSocket4Manager not present" ;

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_send(
				   // Input values,
				   const IPv4&		  src_address,
				   const IPv4&		  dst_address,
				   const string&	  vifname,
				   const uint32_t&	  proto,
				   const uint32_t&	  ttl,
				   const uint32_t&	  tos,
				   const vector<uint8_t>& options,
				   const vector<uint8_t>& payload
				   )
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (_xrsm4 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET4_NULL);
    }
    return _xrsm4->send(src_address, dst_address, vifname,
		       proto, ttl, tos, options, payload);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_send_raw(
				       // Input values,
				       const string&		vifname,
				       const vector<uint8_t>&	packet
				       )
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (_xrsm4 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET4_NULL);
    }
    return _xrsm4->send(vifname, packet);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_register_vif_receiver(
						   // Input values,
						   const string&   router_name,
						   const string&   ifname,
						   const string&   vifname,
						   const uint32_t& proto
						   )
{
    if (_xrsm4 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET4_NULL);
    }
    return _xrsm4->register_vif_receiver(router_name, ifname, vifname, proto);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_unregister_vif_receiver(
						     // Input values,
						     const string& router_name,
						     const string& ifname,
						     const string& vifname,
						     const uint32_t& proto
						     )
{
    if (_xrsm4 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET4_NULL);
    }
    return _xrsm4->unregister_vif_receiver(router_name, ifname, vifname, proto);
}

// ----------------------------------------------------------------------------
// IPv6 Raw Socket related

static const char* XRL_RAW_SOCKET6_NULL = "XrlRawSocket6Manager not present" ;

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_send_raw(
				       // Input values,
				const IPv6&	src_address,
				const IPv6&	dst_address,
				const string&	vif_name,
				const uint32_t&	proto,
				const uint32_t&	tclass,
				const uint32_t&	hoplimit,
				const vector<uint8_t>&	hopopts,
				const vector<uint8_t>&	packet)
{

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

#ifdef notyet
    if (_xrsm6 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);
    }
    return _xrsm6->send(vifname, pktinfo, packet);
#else
    return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);

    UNUSED(src_address);
    UNUSED(dst_address);
    UNUSED(vif_name);
    UNUSED(proto);
    UNUSED(tclass);
    UNUSED(hoplimit);
    UNUSED(hopopts);
    UNUSED(packet);
#endif
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_register_vif_receiver(
						   // Input values,
						   const string&   router_name,
						   const string&   ifname,
						   const string&   vifname,
						   const uint32_t& proto
						   )
{
#ifdef notyet
    if (_xrsm6 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);
    }
    return _xrsm6->register_vif_receiver(router_name, ifname, vifname, proto);
#else
    return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);

    UNUSED(router_name);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(proto);
#endif
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_unregister_vif_receiver(
						     // Input values,
						     const string& router_name,
						     const string& ifname,
						     const string& vifname,
						     const uint32_t& proto
						     )
{
#ifdef notyet
    if (_xrsm6 == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);
    }
    return _xrsm6->unregister_vif_receiver(router_name, ifname, vifname, proto);
#else
    return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET6_NULL);

    UNUSED(router_name);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(proto);
#endif
}

// ----------------------------------------------------------------------------
// Socket Server related

XrlCmdError
XrlFeaTarget::socket4_locator_0_1_find_socket_server_for_addr(
							      const IPv4& addr,
							      string&	  svr
							      )
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    // If we had multiple socket servers we'd look for the right one
    // to use.  At the present time we only have one so this is the
    // one to return
    if (_xss == 0) {
	return XrlCmdError::COMMAND_FAILED("Socket Server is not present.");
    }
    if (_xss->status() != RUNNING) {
	return XrlCmdError::COMMAND_FAILED("Socket Server not running.");
    }
    UNUSED(addr);
    svr = _xss->instance_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_locator_0_1_find_socket_server_for_addr(
							      const IPv6& addr,
							      string&	  svr
							      )
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    // If we had multiple socket servers we'd look for the right one
    // to use.  At the present time we only have one so this is the
    // one to return
    UNUSED(addr);

    if (_xss == 0) {
	return XrlCmdError::COMMAND_FAILED("Socket Server is not present.");
    }
    if (_xss->status() != RUNNING) {
	return XrlCmdError::COMMAND_FAILED("Socket Server not running.");
    }

    svr = _xss->instance_name();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Profiling related

XrlCmdError
XrlFeaTarget::profile_0_1_enable(const string& pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_profile.enable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_disable(const string&	pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_profile.disable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlFeaTarget::profile_0_1_get_entries(const string& pname,
				      const string& instance_name)
{
    debug_msg("profile variable %s instance %s\n", pname.c_str(),
	      instance_name.c_str());

    // Lock and initialize.
    try {
	_profile.lock_log(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    ProfileUtils::transmit_log(pname,
			       dynamic_cast<XrlStdRouter *>(&_xrl_router),
			       instance_name, &_profile);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_clear(const string& pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_profile.clear(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_list(string& info)
{
    debug_msg("\n");
    
    info = _profile.list();
    return XrlCmdError::OKAY();
}
