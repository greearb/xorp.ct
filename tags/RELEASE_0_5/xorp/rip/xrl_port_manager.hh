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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_XRL_PORT_MANAGER_HH__
#define __RIP_XRL_PORT_MANAGER_HH__

#include "libxorp/service.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "port_manager.hh"

template <typename A>
class XrlPortManager
    : public PortManagerBase<A>, public IfMgrHintObserver
{
public:
    XrlPortManager(System<A>& system, IfMgrXrlMirror& ifm)
	: PortManagerBase<A>(system), _ifm(ifm)
    {
	_ifm.attach_hint_observer(this);
    }

    ~XrlPortManager()
    {
	_ifm.detach_hint_observer(this);
    }

    /**
     * Add an address to run RIP on.
     *
     * @param interface to run RIP on.
     * @param vif virtual interface to run RIP on.
     * @param addr address to run RIP on.
     *
     * @return true on success or if address is already running RIP,
     * false on failure.
     */
    bool add_rip_address(const string&	ifname,
			 const string&	vifname,
			 const A&	addr);

    /**
     * Remove an address running RIP.
     *
     * @param interface to run RIP on.
     * @param vif virtual interface to run RIP on.
     * @param addr address to run RIP on.
     *
     * @return true on success or if address is not running RIP, false on
     * otherwise.
     */
    bool remove_rip_address(const string&	ifname,
			    const string&	vifname,
			    const A&		addr);

protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

private:
    XrlPortManager(const XrlPortManager&);		// not implemented
    XrlPortManager& operator=(const XrlPortManager&);	// not implemented

protected:
    IfMgrXrlMirror& _ifm;		// Interface Mirror

};

#endif // __RIP_XRL_PORT_MANAGER_HH__
