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

#ident "$XORP: xorp/fea/xrl_ifmanager.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#include "libxorp/debug.h"
#include "xrl_ifmanager.hh"

static const char* MAX_TRANSACTIONS_HIT =
			"Resource limit on number of pending "
"transactions hit.";

static const char* BAD_ID =
			"Expired or invalid transaction id presented.";


static const char* MISSING_IF =
			"Interface %s does not exist.";

static const char* MISSING_VIF =
			"Vif %s on interface %s does not exist.";

static const char* MISSING_ADDR =
			"Address %s on Vif %s on Interface %s does not exist.";

XrlCmdError
XrlInterfaceManager::get_if_from_config(const IfTree&	it,
					const string&		ifname,
					const IfTreeInterface*&	fi) const
{
    IfTree::IfMap::const_iterator ii = it.get_if(ifname);
    if (ii == iftree().ifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_IF,
						    ifname.c_str()));
    }

    fi = &ii->second;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlInterfaceManager::get_vif_from_config(const IfTree&	it,
					 const string&		ifname,
					 const string&		vif,
					 const IfTreeVif*&		fv) const
{
    IfTree::IfMap::const_iterator ii = it.get_if(ifname);
    if (ii == iftree().ifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_IF,
						    ifname.c_str()));
    }

    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(vif);
    if (vi == ii->second.vifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_VIF,
						    vif.c_str(),
						    ifname.c_str()));
    }

    fv = &vi->second;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlInterfaceManager::get_addr_from_config(const IfTree&	it,
					  const string&		ifname,
					  const string&		vif,
					  const IPv4&		addr,
					  const IfTreeAddr4*&	fa) const
{
    IfTree::IfMap::const_iterator ii = it.get_if(ifname);
    if (ii == iftree().ifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_IF,
						    ifname.c_str()));
    }

    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(vif);
    if (vi == ii->second.vifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_VIF,
						    vif.c_str(),
						    ifname.c_str()));
    }

    IfTreeVif::V4Map::const_iterator ai = vi->second.get_addr(addr);
    if (ai == vi->second.v4addrs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_ADDR,
						    addr.str().c_str(),
						    vif.c_str(),
						    ifname.c_str()));
    }

    fa = &ai->second;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlInterfaceManager::get_addr_from_config(const IfTree&	it,
					  const string&		ifname,
					  const string&		vif,
					  const IPv6&		addr,
					  const IfTreeAddr6*&	fa) const
{
    IfTree::IfMap::const_iterator ii = it.get_if(ifname);
    if (ii == iftree().ifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_IF,
						    ifname.c_str()));
    }

    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(vif);
    if (vi == ii->second.vifs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_VIF,
						    vif.c_str(),
						    ifname.c_str()));
    }

    IfTreeVif::V6Map::const_iterator ai = vi->second.get_addr(addr);
    if (ai == vi->second.v6addrs().end()) {
	return XrlCmdError::COMMAND_FAILED(c_format(MISSING_ADDR,
						    addr.str().c_str(),
						    vif.c_str(),
						    ifname.c_str()));
    }

    fa = &ai->second;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlInterfaceManager::start_transaction(uint32_t& tid)
{
    if (_itm.start(tid))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(MAX_TRANSACTIONS_HIT);
}


XrlCmdError
XrlInterfaceManager::abort_transaction(uint32_t tid)
{
    if (_itm.abort(tid))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(BAD_ID);
}

XrlCmdError
XrlInterfaceManager::add(uint32_t tid, const Operation& op)
{
    uint32_t n_ops = 0;

    if (_itm.size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(BAD_ID);

    if (_itm.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED("Unknown resource shortage");
}

XrlCmdError
XrlInterfaceManager::commit_transaction(uint32_t tid)
{
    // XXX copy existing config ?
    IfTree& local_config = iftree();

    // IfTree backup_config = local_config;
    if (_itm.commit(tid)) {
	if (_itm.error().empty() == false) {
	    return XrlCmdError::COMMAND_FAILED(_itm.error());
	}

	//
	// If we get here we have updated the local copy of the config
	// successfully.
	//
	bool push_success = ifconfig().push_config(local_config);
	local_config.finalize_state();

	// Align with device configuration, so that any stuff that failed
	// in push is not held over in config
	IfTree tmp_config;
	const IfTree& dev_config = ifconfig().pull_config(tmp_config);

	debug_msg("DEV CONFIG %s\n", dev_config.str().c_str());
	debug_msg("LOCAL CONFIG %s\n", local_config.str().c_str());
	
	local_config.align_with(dev_config);
	
	if (push_success) {
	    return XrlCmdError::OKAY();
	}

	// XXX align local_config with h/w config, so we don't have any extra
	// information being held.
	
	// XXX restore
	// if (ifconfig().push_config(backup_config) == false) {
	    // Argh! Die, Die, Die, Die Again
	// }
	return XrlCmdError::COMMAND_FAILED(ifconfig().push_error());
    }

    return XrlCmdError::COMMAND_FAILED(BAD_ID);
}
