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

// $XORP: xorp/libxipc/finder_xrl_target.hh,v 1.7 2003/05/07 23:15:15 mjh Exp $

#ifndef __LIBXIPC_FINDER_XRL_TARGET_HH__
#define __LIBXIPC_FINDER_XRL_TARGET_HH__

#include "finder_base.hh"

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
    XrlCmdError finder_event_notifier_0_1_register_all_event_interest(
		    const string& who);

    XrlCmdError finder_event_notifier_0_1_deregister_all_event_interest(
		    const string& who);

    XrlCmdError finder_event_notifier_0_1_register_class_event_interest(
		    const string& who, const string& class_name);

    XrlCmdError finder_event_notifier_0_1_deregister_class_event_interest(
		    const string& who, const string& class_name);
    
protected:
    Finder& _finder;
};

#endif // __LIBXIPC_FINDER_XRL_TARGET_HH__
