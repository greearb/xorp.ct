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

// $XORP: xorp/fea/xrl_mfea_vif_manager.hh,v 1.12 2004/04/10 07:50:37 pavlin Exp $

#ifndef __FEA_XRL_MFEA_VIF_MANAGER_HH__
#define __FEA_XRL_MFEA_VIF_MANAGER_HH__

#include <map>

#include "libxorp/timer.hh"
#include "libproto/proto_state.hh"

#include "xrl/interfaces/fea_ifmgr_xif.hh"

#define IF_EVENT_CREATED 1
#define IF_EVENT_DELETED 2
#define IF_EVENT_CHANGED 3

class EventLoop;
class MfeaNode;
class Vif;
class XrlRouter;

/**
 * @short XrlMfeaVifManager keeps track of the VIFs currently enabled
 * in the FEA
 *
 * The XrlMfeaNode has a single XrlMfeaVifManager instance, which registers
 * with the FEA to discover the VIFs on this router and their
 * IP addresses and prefixes. When the VIFs or their configuration in
 * the FEA change, the XrlMfeaVifManager will be notified, and it will update
 * the MfeaNode (itself also included in the XrlMfeaNode) appropriately.
 * Then the MfeaNode may inform all multicast-related modules that have
 * registered interest with the MFEA to receive VIF-related updates.
 */
class XrlMfeaVifManager : public ProtoState {
public:
    /**
     * XrlMfeaVifManager constructor.
     *
     * @param mfea_node the @ref MfeaNode this manager corresponds to.
     * @param eventloop the event loop to use.
     * @param xrl_router the XRL router to use.
     */
    XrlMfeaVifManager(MfeaNode& mfea_node, EventLoop& eventloop,
		      XrlRouter* xrl_router);

    /**
     * XrlMfeaVifManager destructor
     */
    virtual ~XrlMfeaVifManager();

    /**
     * Start operation.
     * 
     * Start the process of registering with the FEA, etc.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	start();

    /**
     * Stop operation.
     * 
     * Gracefully stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	stop();

    /**
     * Get the address family.
     * 
     * @return the address family (e.g., AF_INET or AF_INET6 for IPv4 and
     * IPv6 respectively).
     */
    int family() const;

    /**
     * Set test-mode - don't try to communicate with the FEA.
     */
    void no_fea() { _no_fea = true; }

    /**
     * The state of the XrlMfeaVifManager. It it hasn't yet successfully
     * registered with the FEA, it will be in INITIALIZING state. If
     * it has permanently failed to register with the FEA it will be
     * in FAILED state. Otherwise it should be in READY state.
     */
    enum State { INITIALIZING, READY, FAILED };

    /**
     * Get the state of the XrlMfeaVifManager.
     * 
     * @return the state of the XrlMfeaVifManager. 
     * @see State
     */
    State state() const { return _state; }

    /**
     * Update the status of a physical interface.
     * 
     * This method is called when receiving an XRL from the FEA
     * indicating that a physical interface on the router has been added,
     * deleted, or reconfigured.
     * 
     * @param ifname the name of the physical interface that changed.
     * @param event the event that occured. Should be one of the following:
     * IF_EVENT_CREATED, IF_EVENT_DELETED, or IF_EVENT_CHANGED.
     */
    void interface_update(const string& ifname, const uint32_t& event);

    /**
     * Update the status of a virtual interface.
     * 
     * This method is called when receiving an XRL from the FEA
     * indicating that a virtual interface on the router has been added,
     * deleted, or reconfigured.
     * 
     * @param ifname the name of the physical interface on which the
     * virtual interface resides.
     * @param vifname the name of the virtual interface that changed.
     * @param event the event that occured. Should be one of the following:
     * IF_EVENT_CREATED, IF_EVENT_DELETED, or IF_EVENT_CHANGED.
     */
    void vif_update(const string& ifname, const string& vifname,
		    const uint32_t& event);

    /**
     * Update the IPv4 address of a virtual interface.
     * 
     * This method is called when receiving an XRL from the FEA
     * indicating that a virtual interface has undergone an address
     * change. An IPv4 address (and associated prefix length) has
     * been added, deleted, or reconfigured on this VIF.
     * 
     * @param ifname the name of the interface containing the VIF.
     * @param vifname the name of the VIF on which the address change occured.
     * @param addr the address that was added or deleted.
     * @param event the event that occured. Should be one of the following:
     * IF_EVENT_CREATED or IF_EVENT_DELETED.
     */
    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4& addr,
			 const uint32_t& event);

    /**
     * Update the IPv6 address of a virtual interface.
     * 
     * This method is called when receiving an XRL from the FEA
     * indicating that a virtual interface has undergone an address
     * change. An IPv6 address (and associated prefix length) has
     * been added, deleted, or reconfigured on this VIF.
     *
     * @param ifname the name of the interface containing the VIF.
     * @param vifname the name of the VIF on which the address change occured.
     * @param addr the address that was added or deleted.
     * @param event the event that occured. Should be one of the following:
     * IF_EVENT_CREATED or IF_EVENT_DELETED.
     */
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6& addr,
			 const uint32_t& event);

    /**
     * The interface updates have been completed.
     */
    void updates_completed();
    
private:
    void update_state();
    void set_vif_state();
    
    void clean_out_old_state();
    void ifmgr_client_send_unregister_system_interfaces_client_cb(const XrlError& e);
    void register_if_spy();
    void ifmgr_client_send_register_system_interfaces_client_cb(const XrlError& e);
    void ifmgr_client_send_get_system_interface_names_cb(const XrlError& e,
							 const XrlAtomList *alist);
    void ifmgr_client_send_get_system_vif_names_cb(const XrlError& e,
						   const XrlAtomList *alist,
						   string ifname);
    void ifmgr_client_send_get_system_vif_pif_index_cb(const XrlError& e,
						       const uint32_t* pif_index,
						       string ifname,
						       string vifname);
    void ifmgr_client_send_get_system_vif_flags_cb(const XrlError& e,
						   const bool* enabled,
						   const bool* broadcast,
						   const bool* loopback,
						   const bool* point_to_point,
						   const bool* multicast,
						   string ifname,
						   string vifname);
    void ifmgr_client_send_get_system_vif_addresses4_cb(const XrlError& e,
							const XrlAtomList *alist,
							string ifname,
							string vifname);
    void ifmgr_client_send_get_system_vif_addresses6_cb(const XrlError& e,
							const XrlAtomList *alist,
							string ifname,
							string vifname);
    void interface_deleted(const string& ifname);
    void vif_deleted(const string& ifname, const string& vifname);
    void vif_created(const string& ifname, const string& vifname);
    void vifaddr4_created(const string& ifname, const string& vifname,
			  const IPv4& addr);
    void vifaddr6_created(const string& ifname, const string& vifname,
			  const IPv6& addr);
    void ifmgr_client_send_get_system_address_flags4_cb(const XrlError& e,
							const bool* enabled,
							const bool* broadcast,
							const bool* loopback,
							const bool* point_to_point,
							const bool* multicast,
							string ifname,
							string vifname,
							IPv4 addr);
    void ifmgr_client_send_get_system_address_flags6_cb(const XrlError& e,
							const bool* enabled,
							const bool* loopback,
							const bool* point_to_point,
							const bool* multicast,
							string ifname,
							string vifname,
							IPv6 addr);
    void ifmgr_client_send_get_system_prefix4_cb(const XrlError& e,
						 const uint32_t* prefix_len,
						 string ifname,
						 string vifname,
						 IPv4 addr);
    void ifmgr_client_send_get_system_prefix6_cb(const XrlError& e,
						 const uint32_t* prefix_len,
						 string ifname,
						 string vifname,
						 IPv6 addr);
    void ifmgr_client_send_get_system_broadcast4_cb(const XrlError& e,
						    const IPv4* broadcast,
						    string ifname,
						    string vifname,
						    IPv4 addr);
    void ifmgr_client_send_get_system_endpoint4_cb(const XrlError& e,
						   const IPv4* endpoint,
						   string ifname,
						   string vifname,
						   IPv4 addr);
    void ifmgr_client_send_get_system_endpoint6_cb(const XrlError& e,
						   const IPv6* endpoint,
						   string ifname,
						   string vifname,
						   IPv6 addr);
    void vifaddr4_deleted(const string& ifname, const string& vifname,
			  const IPv4& addr);
    void vifaddr6_deleted(const string& ifname, const string& vifname,
			  const IPv6& addr);
    
    MfeaNode&		_mfea_node;
    EventLoop&		_eventloop;
    XrlRouter&		_xrl_router;
    XrlIfmgrV0p1Client	_ifmgr_client;
    
    bool		_no_fea;
    XorpTimer		_register_retry_timer;
    State		_state;
    
    // The following variables keep track of how many answers we're
    // still expecting from various pipelined queries to the FEA.
    size_t		_interfaces_remaining;
    size_t		_vifs_remaining;
    size_t		_addrs_remaining;
    
    // The maps with the interfaces and vifs
    map<string, Vif *>	_vifs_by_name;
    multimap<string, Vif *> _vifs_by_interface;
    
    string _fea_target_name;	// The FEA target name
};

#endif // __FEA_XRL_MFEA_VIF_MANAGER_HH__
