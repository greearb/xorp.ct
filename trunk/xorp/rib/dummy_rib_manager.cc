// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/rib/dummy_rib_manager.cc,v 1.13 2004/09/17 14:00:03 abittau Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"
#include "libxorp/profile.hh"

#include "dummy_rib_manager.hh"

RibManager::RibManager()
{
}

RibManager::~RibManager()
{
}

int
RibManager::start()
{
    return XORP_OK;
}

int
RibManager::stop()
{
    return XORP_OK;
}

ProcessStatus
RibManager::status(string& reason) const
{
    UNUSED(reason);

    return PROC_READY;
}

void
RibManager::register_interest_in_target(const string& target_class)
{
    UNUSED(target_class);
}

int
RibManager::new_vif(const string& vifname, const Vif& vif, string& err)
{
    UNUSED(vifname);
    UNUSED(vif);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::delete_vif(const string& vifname, string& err)
{
    UNUSED(vifname);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::set_vif_flags(const string& vifname,
			  bool is_p2p,
			  bool is_loopback,
			  bool is_multicast,
			  bool is_broadcast,
			  bool is_up,
			  string& err)
{
    UNUSED(vifname);
    UNUSED(is_p2p);
    UNUSED(is_loopback);
    UNUSED(is_multicast);
    UNUSED(is_broadcast);
    UNUSED(is_up);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::add_vif_address(const string& vifname,
			    const IPv4& addr,
			    const IPv4Net& subnet,
			    const IPv4& broadcast_addr,
			    const IPv4& peer_addr,
			    string& err)
{
    UNUSED(vifname);
    UNUSED(addr);
    UNUSED(subnet);
    UNUSED(broadcast_addr);
    UNUSED(peer_addr);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::add_vif_address(const string& vifname,
			    const IPv6& addr,
			    const IPv6Net& subnet,
			    const IPv6& peer_addr,			    
			    string& err)
{
    UNUSED(vifname);
    UNUSED(addr);
    UNUSED(subnet);
    UNUSED(peer_addr);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::delete_vif_address(const string& vifname,
			       const IPv4& addr,
			       string& err)
{
    UNUSED(vifname);
    UNUSED(addr);
    UNUSED(err);

    return XORP_OK;
}

int
RibManager::delete_vif_address(const string& vifname,
			       const IPv6& addr,
			       string& err)
{
    UNUSED(vifname);
    UNUSED(addr);
    UNUSED(err);

    return XORP_OK;
}

void
RibManager::make_errors_fatal()
{
}

void
RibManager::target_death(const string& target_class,
			 const string& target_instance)
{
    UNUSED(target_class);
    UNUSED(target_instance);
}

int
RibManager::add_redist_xrl_output4(const string&	/* target_name */,
				   const string&	/* from_protocol */,
				   bool			/* unicast */,
				   bool			/* multicast */,
				   const string&	/* cookie */,
				   bool			/* is_xrl_transaction_output */)
{
    return XORP_OK;
}

int
RibManager::add_redist_xrl_output6(const string&	/* target_name */,
				   const string&	/* from_protocol */,
				   bool			/* unicast */,
				   bool			/* multicast */,
				   const string&	/* cookie */,
				   bool			/* is_xrl_transaction_output */)
{
    return XORP_OK;
}

int
RibManager::delete_redist_xrl_output4(const string&	/* target_name */,
				      const string&	/* from_protocol */,
				      bool		/* unicast */,
				      bool		/* multicast */,
				      const string&	/* cookie */,
				      bool		/* is_xrl_transaction_output */)
{
    return XORP_OK;
}

int
RibManager::delete_redist_xrl_output6(const string&	/* target_name */,
				      const string&	/* from_protocol */,
				      bool		/* unicast */,
				      bool		/* multicast */,
				      const string&	/* cookie */,
				      bool		/* is_xrl_transaction_output */)
{
    return XORP_OK;
}

void
RibManager::configure_filter(const uint32_t&	/* filter */,
			     const string&	/* conf */)
{
}

void
RibManager::reset_filter(const uint32_t&	/* filter */)
{
}

void
RibManager::push_routes()
{
}

void
RibManager::insert_policy_redist_tags(const string& /* protocol */,
                                      const PolicyTags& /* tags */)
{
}

void 
RibManager::reset_policy_redist_tags()
{
}

PolicyFilters&
RibManager::policy_filters()
{
    return _policy_filters;
}

PolicyRedistMap&
RibManager::policy_redist_map() 
{ 
    return _policy_redist_map;
}

Profile&
RibManager::profile() 
{ 
    return _profile;
}

XrlStdRouter&
RibManager::xrl_router()
{
    // hope xrl router was set
    return *_xrl_router;
}

void
RibManager::set_xrl_router(XrlStdRouter& rtr)
{
    _xrl_router = &rtr;
}
