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

// $XORP: xorp/libxipc/finder_ng_xrl_target.hh,v 1.3 2003/02/26 18:33:38 hodson Exp $

#ifndef __LIBXIPC_FINDER_XRL_TGT_HH__
#define __LIBXIPC_FINDER_XRL_TGT_HH__

#include "finder_base.hh"

class FinderNG;

class FinderNGXrlTarget : public XrlFinderTargetBase {
public:
    FinderNGXrlTarget(FinderNG& finder);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(string&	name);
    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(string&	version);

    /**
     *  Fails if target_name is already registered. The target_name must
     *  support the finder_client interface in order to be able to process
     *  messages from the finder.
     */
    XrlCmdError finder_0_1_register_finder_client(const string&	target_name,
						  const string&	class_name,
						  const string& in_cookie,
						  string&	out_cookie);  

    XrlCmdError finder_0_1_unregister_finder_client(const string& target_name);

    XrlCmdError finder_0_1_set_finder_client_enabled(const string& target_name,
						     const bool&   en);

    XrlCmdError finder_0_1_finder_client_enabled(const string& target_name,
						 bool&         en);

    /**
     *  Add resolved Xrl into system, fails if xrl is already registered.
     */
    XrlCmdError finder_0_1_add_xrl(const string& xrl, 
				   const string& protocol_name, 
				   const string& protocol_args, 
				   string&	 resolved_xrl_method_name);

    /**
     *  Remove xrl
     */
    XrlCmdError finder_0_1_remove_xrl(const string&	xrl);

    /**
     *  Resolve Xrl
     */
    XrlCmdError finder_0_1_resolve_xrl(const string&	xrl,
				       XrlAtomList&	resolutions);
    
    /**
     *  Get list of registered Xrl targets
     */
    XrlCmdError finder_0_1_get_xrl_targets(XrlAtomList&	target_names);

    /**
     *  Get list of Xrls registered by target
     */
    XrlCmdError finder_0_1_get_xrls_registered_by(const string&	target_name, 
						  XrlAtomList&	xrls);

    XrlCmdError finder_0_1_get_ipv4_permitted_hosts(XrlAtomList& ipv4s);

    XrlCmdError finder_0_1_get_ipv4_permitted_nets(XrlAtomList&  ipv4nets);

    XrlCmdError finder_0_1_get_ipv6_permitted_hosts(XrlAtomList& ipv6s);

    XrlCmdError finder_0_1_get_ipv6_permitted_nets(XrlAtomList&  ipv6nets);
    
protected:
    FinderNG& _finder;
};

#endif // __LIBXIPC_FINDER_XRL_TGT_HH__
