// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rib/vifmanager.hh,v 1.2 2003/02/06 22:21:32 hodson Exp $

#ifndef __RIB_VIFMANAGER_HH__
#define __RIB_VIFMANAGER_HH__

#include <map>
#include "libxorp/vif.hh"
#include "libxipc/xrl_router.hh"
#include "xrl/interfaces/fea_ifmgr_xif.hh"

#define IF_EVENT_CREATED 1
#define IF_EVENT_DELETED 2
#define IF_EVENT_CHANGED 3

class InterfaceMonitor {
public:
    /**
     * The state of the InterfaceMonitor.  It it hasn't yet successfully
     * registered with the FEA, it will be in INITIALIZING state.  If
     * it has permanently failed to register with the FEA it will be
     * in FAILED state.  Otherwise it should be in READY state.
     */
    enum State { INITIALIZING, READY, FAILED };

    /**
     * InterfaceMonitor constructor
     *
     * @param xrl_rtr this process's XRL router.
     * @param eventloop this process's EventLoop.
     */
    InterfaceMonitor(XrlRouter& xrl_rtr, EventLoop& eventloop);

    /**
     * InterfaceMonitor destructor
     */
    ~InterfaceMonitor();

    /**
     * Start the process of registering with the FEA, etc
     */
    void start();

    /**
     * @returns the state of the InterfaceMonitor. 
     * @see InterfaceMonitor::State
     */
    State state() const { return _state; }

    /**
     * interface_update is called when receiving an XRL from the FEA
     * indicating that a physical interface on the router has been added,
     * deleted, or reconfigured 
     *
     * @param ifname the name of the physical interface that changed.
     * @param event the event that occured. One of IF_EVENT_CREATED,
     * IF_EVENT_DELETED, or IF_EVENT_CHANGED
     */
    void interface_update(const string& ifname,
			  const uint32_t& event);

    /**
     * vif_update is called when receiving an XRL from the FEA
     * indicating that a virtual interface on the router has been added,
     * deleted, or reconfigured 
     *
     * @param ifname the name of the physical interface on which the
     * virtual interface resides.
     * @param vifname the name of the virtual interface that changed.
     * @param event the event that occured. One of IF_EVENT_CREATED,
     * IF_EVENT_DELETED, or IF_EVENT_CHANGED 
     */
    void vif_update(const string& ifname,
		    const string& vifname, const uint32_t& event);

    /**
     * vifaddr4_update is called when receiving an XRL from the FEA
     * indicating that a virtual interface has undergone an address
     * change.  An IPv4 address (and associated prefix length) has
     * been added, deleted, or reconfigured on this VIF.
     *
     * @param ifname the name of the interface containing the VIF
     * @param vifname the name of the VIF on which the address change occured.
     * @param addr the address that was added or deleted.
     * @param event the event that occured. One of IF_EVENT_CREATED or
     * IF_EVENT_DELETED 
     */
    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4& addr,
			 const uint32_t& event);

    /**
     * vifaddr6_update is called when receiving an XRL from the FEA
     * indicating that a virtual interface has undergone an address
     * change.  An IPv6 address (and associated prefix length) has
     * been added, deleted, or reconfigured on this VIF.
     *
     * @param ifname the name of the interface containing the VIF
     * @param vifname the name of the VIF on which the address change occured.
     * @param addr the address that was added or deleted.
     * @param event the event that occured. One of IF_EVENT_CREATED or
     * IF_EVENT_DELETED 
     */
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6& addr,
			 const uint32_t& event);
private:
    void clean_out_old_state();
    void clean_out_old_state_done(const XrlError& e);
    void register_if_spy();
    void register_if_spy_done(const XrlError& e);
    void interface_names_done(const XrlError& e, const XrlAtomList* alist);
    void vif_names_done(const XrlError& e, const XrlAtomList* alist,
			string ifname);
    void get_all_vifaddr4_done(const XrlError& e, const XrlAtomList* alist,
			       string ifname, string vifname);

    void interface_deleted(const string& ifname);
    void vif_deleted(const string& ifname, const string& vifname);
    void vif_created(const string& ifname, const string& vifname);
    void vifaddr4_created(const string& ifname, const string& vifname,
			  const IPv4& addr);
    void vifaddr4_done(const XrlError& e, const uint32_t* prefix_len,
		       string ifname, string vifname,
		       IPv4 addr);
    void vifaddr6_created(const string& ifname, const string& vifname,
			  const IPv6& addr);
    void vifaddr6_done(const XrlError& e, const uint32_t* prefix_len,
		       string ifname, string vifname,
		       IPv6 addr);
    void vifaddr4_deleted(const string& ifname, const string& vifname,
			  const IPv4& addr);
    void vifaddr6_deleted(const string& ifname, const string& vifname,
			  const IPv6& addr);

    XrlRouter &_xrl_rtr;
    EventLoop &_eventloop;
    XrlIfmgrV0p1Client _ifmgr_client;
    XorpTimer _register_retry_timer;
    State _state;
    int _register_retry_counter;

    /* the following variables keep track of how many answers we're
       still expecting from various pipelined queries to the FEA */
    int _interfaces_remaining;
    int _vifs_remaining;
    int _addrs_remaining;

    map <string, Vif*> _vifs_by_name;
    multimap <string, Vif*> _vifs_by_interface;
};

#endif // __RIB_VIFMANAGER_HH__
