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

// $XORP: xorp/fea/xrl_ifmanager.hh,v 1.6 2003/05/14 09:37:55 pavlin Exp $

#ifndef __FEA_XRL_IFMANAGER_HH__
#define __FEA_XRL_IFMANAGER_HH__

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"
#include "ifmanager.hh"
#include "ifmanager_transaction.hh"

/**
 * Helper class for helping with Interface configuration transactions
 * via an Xrl interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the InterfaceManager
 * class.
 */
class XrlInterfaceManager
{
public:
    typedef InterfaceTransactionManager::Operation Operation;

    /**
     * Constructor
     *
     * @param e the EventLoop.
     * @param ifm the InterfaceManager object.
     * @param max_ops the maximum number of operations pending.
     */
    XrlInterfaceManager(EventLoop&	     e,
			InterfaceManager& ifm,
			uint32_t	     max_ops = 200)
	: _itm(e, 5000, 10), _ifm(ifm), _max_ops(max_ops)
    {}

    /**
     * get the status of the interface manager.
     *
     * @param reason the human-readable reason for any failure.
     * @return the status of the interface manager.
     */
    ProcessStatus status(string& reason) const;

    //
    // Transaction related methods
    //
    XrlCmdError start_transaction(uint32_t& tid);

    XrlCmdError commit_transaction(uint32_t tid);

    XrlCmdError abort_transaction(uint32_t tid);

    XrlCmdError add(uint32_t tid, const Operation& op);

    //
    // Miscellaneous helper methods
    //
    inline XrlCmdError get_if(const string&	   ifname,
			      const IfTreeInterface*& fi) const;

    inline XrlCmdError get_vif(const string&  ifname,
			       const string&  vifname,
			       const IfTreeVif*& fv) const;

    inline XrlCmdError get_addr(const string&	 ifname,
				const string&	 vifname,
				const IPv4&	 addr,
				const IfTreeAddr4*& fa) const;

    inline XrlCmdError get_addr(const string&	 ifname,
				const string&	 vifname,
				const IPv6&	 addr,
				const IfTreeAddr6*& fa) const;

    inline XrlCmdError pull_config_get_if(const string& ifname,
					  const IfTreeInterface*& fi) const;

    inline XrlCmdError pull_config_get_vif(const string&  ifname,
					   const string&  vifname,
					   const IfTreeVif*& fv) const;

    inline XrlCmdError pull_config_get_addr(const string&	ifname,
					    const string&	vifname,
					    const IPv4&		addr,
					    const IfTreeAddr4*&	fa) const;

    inline XrlCmdError pull_config_get_addr(const string&	ifname,
					const string&		vifname,
					const IPv6&		addr,
					const IfTreeAddr6*&	fa) const;

    inline XrlCmdError addr_valid(const string& ifname,
				  const string& vifname,
				  const IPv4&	addr,
				  const string& descr,
				  const IPv4&   validate_addr);

    inline XrlCmdError addr_valid(const string& ifname,
				  const string& vifname,
				  const IPv6&	addr,
				  const string& descr,
				  const IPv6&   validate_addr);

    inline IfTree& iftree() const	{ return _ifm.iftree(); }

    inline IfConfig& ifconfig() const		{ return _ifm.ifc(); }

protected:
    XrlCmdError get_if_from_config(const IfTree&	it,
				   const string&	ifname,
				   const IfTreeInterface*&	fi) const;

    XrlCmdError get_vif_from_config(const IfTree&	it,
				    const string&	ifname,
				    const string&	vifname,
				    const IfTreeVif*&	fv) const;

    XrlCmdError get_addr_from_config(const IfTree&	it,
				     const string&	ifname,
				     const string&	vifname,
				     const IPv4&	addr,
				     const IfTreeAddr4*&	fa) const;

    XrlCmdError get_addr_from_config(const IfTree&	it,
				     const string&	ifname,
				     const string&	vifname,
				     const IPv6&	addr,
				     const IfTreeAddr6*&	fa) const;

protected:
    InterfaceTransactionManager	_itm;
    InterfaceManager&		_ifm;
    uint32_t			_max_ops;
    ProcessStatus               _status, _prev_status;
};

inline XrlCmdError
XrlInterfaceManager::get_if(const string&	  ifname,
			    const IfTreeInterface*& fi) const
{
    return get_if_from_config(iftree(), ifname, fi);
}

inline XrlCmdError
XrlInterfaceManager::get_vif(const string&  ifname,
			     const string&  vif,
			     const IfTreeVif*& fv) const
{
    return get_vif_from_config(iftree(), ifname, vif, fv);
}

inline XrlCmdError
XrlInterfaceManager::get_addr(const string&	 ifname,
			      const string&	 vif,
			      const IPv4&	 addr,
			      const IfTreeAddr4*& fa) const
{
    return get_addr_from_config(iftree(), ifname, vif, addr, fa);
}

inline XrlCmdError
XrlInterfaceManager::get_addr(const string&	 ifname,
			      const string&	 vif,
			      const IPv6&	 addr,
			      const IfTreeAddr6*& fa) const
{
    return get_addr_from_config(iftree(), ifname, vif, addr, fa);
}

inline XrlCmdError
XrlInterfaceManager::pull_config_get_if(const string&	ifname,
			    const IfTreeInterface*&	fi) const
{
    const IfTree& it = ifconfig().pull_config();
    return get_if_from_config(it, ifname, fi);
}

inline XrlCmdError
XrlInterfaceManager::pull_config_get_vif(const string&	ifname,
				     const string&	vif,
				     const IfTreeVif*&	fv) const
{
    const IfTree& it = ifconfig().pull_config();
    return get_vif_from_config(it, ifname, vif, fv);
}

inline XrlCmdError
XrlInterfaceManager::pull_config_get_addr(const string&	ifname,
				      const string&	vif,
				      const IPv4&	addr,
				      const IfTreeAddr4*&	fa) const
{
    const IfTree& it = ifconfig().pull_config();
    return get_addr_from_config(it, ifname, vif, addr, fa);
}

inline XrlCmdError
XrlInterfaceManager::pull_config_get_addr(const string&	ifname,
				      const string&	vif,
				      const IPv6&	addr,
				      const IfTreeAddr6*&	fa) const
{
    const IfTree& it = ifconfig().pull_config();
    return get_addr_from_config(it, ifname, vif, addr, fa);
}

inline XrlCmdError
XrlInterfaceManager::addr_valid(const string& ifname,
				const string& vifname,
				const IPv4&   addr,
				const string& descr,
				const IPv4&   validate_addr)
{
    if (validate_addr != IPv4::ZERO())
	return XrlCmdError::OKAY();
    return
	XrlCmdError::COMMAND_FAILED(c_format("No %s address associated with "
					     "address %s Vif %s on "
					     "Interface %s.",
					     descr.c_str(), addr.str().c_str(),
					     vifname.c_str(), ifname.c_str()));
}

inline XrlCmdError
XrlInterfaceManager::addr_valid(const string& ifname,
				const string& vifname,
				const IPv6&   addr,
				const string& descr,
				const IPv6&   validate_addr)
{
    if (validate_addr != IPv6::ZERO())
	return XrlCmdError::OKAY();
    return
	XrlCmdError::COMMAND_FAILED(c_format("No %s address associated with "
					     "address %s Vif %s on "
					     "Interface %s.",
					     descr.c_str(), addr.str().c_str(),
					     vifname.c_str(), ifname.c_str()));
}

#endif // __FEA_XRL_IFMANAGER_HH__
