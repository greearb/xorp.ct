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

// $XORP: xorp/rib/dummy_rib_manager.hh,v 1.1 2003/09/27 22:32:45 mjh Exp $

#ifndef __RIB_DUMMY_RIB_MANAGER_HH__
#define __RIB_DUMMY_RIB_MANAGER_HH__

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libproto/proto_state.hh"

#include "libxipc/xrl_std_router.hh"

#include "rib.hh"
#include "rib_client.hh"
#include "register_server.hh"
#include "vifmanager.hh"
#include "xrl_target.hh"

/**
 * @short Main top-level class containing RIBs and main eventloop.
 *
 * The single RibManager class instance is the top-level class in the
 * RIB process from which everything else is built and run.  It
 * contains the four RIBs for IPv4 unicast routes, IPv4 multicast
 * routes, IPv6 unicast routes and IPv6 multicast routes.  It also
 * contains the RIB's main eventloop.  
 */
class RibManager : public ProtoState {
public:
    /**
     * RibManager constructor
     * 
     * @param eventloop the event loop to user.
     * @param xrl_std_router the XRL router to use.
     */
    RibManager();

    /**
     * RibManager destructor
     */
    ~RibManager();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	start();
    
    /**
     * Stop operation.
     * 
     * Gracefully stop the RIB.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	stop();

    /**
     * Periodic Status Update 
     *
     * @return true to reschedule next status check.
     */
    bool status_updater();

    /**
     * Check status of RIB process.
     *
     * @return process status code
     */
    ProcessStatus status(string& reason) const; 

    /**
     * Inform the RIB about the existence of a Virtual Interface.
     * Note that it is an error to add twice a vif with the same vifname.
     *
     * @see Vif
     * @param vifname the name of the VIF, as understood by the FEA.
     * @param vif Vif class instance giving the information about this vif.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.  
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int new_vif(const string& vifname, const Vif& vif, string& err);

    /**
     * delete_vif is called to inform all the RIBs that a virtual
     * interface that they previously knew about has been deleted.
     *
     * @param vifname the name of the VIF that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int delete_vif(const string& vifname, string& err);

    /**
     * add_vif_address is called to inform all the RIBs that a new IPv4
     * address has been added to a virtual interface.
     *
     * @param vifname the name of the VIF that the address was added to.
     * @param addr the new address.
     * @param net the subnet (masked address) that the new address
     * resides on.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int add_vif_address(const string& vifname, 
			const IPv4& addr,
			const IPNet<IPv4>& net,
			string& err);

    /**
     * delete_vif_address is called to inform all the RIBs that an IPv4
     * address that they previously know about has been deleted from a
     * specific VIF.
     *
     * @param vifname the name of the VIF that the address was deleted from.
     * @param addr the address that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int delete_vif_address(const string& vifname, 
			   const IPv4& addr,
			   string& err);

    /**
     * add_vif_address is called to inform all the RIBs that a new IPv6
     * address has been added to a virtual interface.
     *
     * @param vifname the name of the VIF that the address was added to.
     * @param addr the new address.
     * @param net the subnet (masked address) that the new address
     * resides on.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int add_vif_address(const string& vifname,
			const IPv6& addr,
			const IPNet<IPv6>& net,
			string& err);

    /**
     * delete_vif_address is called to inform all the RIBs that an IPv6
     * address that they previously know about has been deleted from a
     * specific VIF.
     *
     * @param vifname the name of the VIF that the address was deleted from.
     * @param addr the address that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int delete_vif_address(const string& vifname, 
			   const IPv6& addr,
			   string& err);
    
    
    /**
     * Find a RIB client.
     * 
     * Find a RIB client for a given target name, address family, and
     * unicast/multicast flags.
     * 
     * @param target_name the target name of the RIB client.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param unicast true if a client for the unicast RIB.
     * @param multicast true if a client for the multicast RIB.
     * @return a pointer to a valid @ref RibClient if found, otherwise NULL.
     */
    RibClient *find_rib_client(const string& target_name, int family,
			       bool unicast, bool multicast);
    
    /**
     * Add a RIB client.
     * 
     * Add a RIB client for a given target name, address family, and
     * unicast/multicast flags.
     * 
     * @param target_name the target name of the RIB client.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param unicast true if a client for the unicast RIB.
     * @param multicast true if a client for the multicast RIB.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_rib_client(const string& target_name, int family,
		       bool unicast, bool multicast);

    /**
     * Delete a RIB client.
     * 
     * Delete a RIB client for a given target name, address family, and
     * unicast/multicast flags.
     * 
     * @param target_name the target name of the RIB client.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param unicast true if a client for the unicast RIB.
     * @param multicast true if a client for the multicast RIB.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_rib_client(const string& target_name, int family,
			  bool unicast, bool multicast);

    /**
     * Enable a RIB client.
     * 
     * Enable a RIB client for a given target name, address family, and
     * unicast/multicast flags.
     * 
     * @param target_name the target name of the RIB client.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param unicast true if a client for the unicast RIB.
     * @param multicast true if a client for the multicast RIB.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_rib_client(const string& target_name, int family,
			  bool unicast, bool multicast);

    /**
     * Disable a RIB client.
     * 
     * Disable a RIB client for a given target name, address family, and
     * unicast/multicast flags.
     * 
     * @param target_name the target name of the RIB client.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param unicast true if a client for the unicast RIB.
     * @param multicast true if a client for the multicast RIB.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int disable_rib_client(const string& target_name, int family,
			   bool unicast, bool multicast);
    
    /**
     * Don't try to communicate with the FEA.
     * 
     * Note that this method will be obsoleted in the future, and will
     * be replaced with cleaner interface.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int no_fea();

    /**
     * Make some errors we'd normally mask fatal.  Should be used for
     * testing only.
     */
    void make_errors_fatal();

    /**
     * Register Interest in an XRL target so we can monitor process
     * births and deaths and clean up appropriately when things die.
     *
     * @param tgt_class the XRL Target Class we're interested in.  
     */
    void register_interest_in_target(const string& tgt_class);

    /**
     * Called in response to registering interest in an XRL target
     *
     * @param e XRL Response code.
     */
    void register_interest_in_target_done(const XrlError& e);

    void target_death(const string& tgt_class, const string& tgt_instance);
private:
};

#endif // __RIB_DUMMY_RIB_MANAGER_HH__
