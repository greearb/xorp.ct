// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/finder_xrl_target.hh,v 1.19 2008/07/23 05:10:42 pavlin Exp $

#ifndef __LIBXIPC_FINDER_XRL_TARGET_HH__
#define __LIBXIPC_FINDER_XRL_TARGET_HH__

#include "xrl/targets/finder_base.hh"

class Finder;

class FinderXrlTarget : public XrlFinderTargetBase {
public:
    FinderXrlTarget(Finder& finder);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(string&	name);
    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(string&	version);
    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    /**
     *  Request Xrl Target to shutdown
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Fails if target_name is already registered. The target_name must
     *  support the finder_client interface in order to be able to process
     *  messages from the finder.
     */
    XrlCmdError finder_0_2_register_finder_client(const string&	target_name,
						  const string&	class_name,
						  const bool&	singleton,
						  const string& in_cookie,
						  string&	out_cookie);

    XrlCmdError finder_0_2_unregister_finder_client(const string& target_name);

    XrlCmdError finder_0_2_set_finder_client_enabled(const string& target_name,
						     const bool&   en);

    XrlCmdError finder_0_2_finder_client_enabled(const string& target_name,
						 bool&         en);

    /**
     *  Add resolved Xrl into system, fails if xrl is already registered.
     */
    XrlCmdError finder_0_2_add_xrl(const string& xrl,
				   const string& protocol_name,
				   const string& protocol_args,
				   string&	 resolved_xrl_method_name);

    /**
     *  Remove xrl
     */
    XrlCmdError finder_0_2_remove_xrl(const string&	xrl);

    /**
     *  Resolve Xrl
     */
    XrlCmdError finder_0_2_resolve_xrl(const string&	xrl,
				       XrlAtomList&	resolutions);

    /**
     *  Get list of registered Xrl targets
     */
    XrlCmdError finder_0_2_get_xrl_targets(XrlAtomList&	target_names);

    /**
     *  Get list of Xrls registered by target
     */
    XrlCmdError finder_0_2_get_xrls_registered_by(const string&	target_name,
						  XrlAtomList&	xrls);

    XrlCmdError finder_0_2_get_ipv4_permitted_hosts(XrlAtomList& ipv4s);

    XrlCmdError finder_0_2_get_ipv4_permitted_nets(XrlAtomList&  ipv4nets);

    XrlCmdError finder_0_2_get_ipv6_permitted_hosts(XrlAtomList& ipv6s);

    XrlCmdError finder_0_2_get_ipv6_permitted_nets(XrlAtomList&  ipv6nets);

    /**
     * Event notifier interface.
     */
    XrlCmdError finder_event_notifier_0_1_register_class_event_interest(
		    const string& who, const string& class_name);

    XrlCmdError finder_event_notifier_0_1_deregister_class_event_interest(
		    const string& who, const string& class_name);

    XrlCmdError finder_event_notifier_0_1_register_instance_event_interest(
		    const string& who, const string& instance_name);

    XrlCmdError finder_event_notifier_0_1_deregister_instance_event_interest(
		    const string& who, const string& instance_name);

protected:
    Finder& _finder;
};

#endif // __LIBXIPC_FINDER_XRL_TARGET_HH__
