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

// $XORP: xorp/rib/dummy_rib_manager.hh,v 1.8 2004/05/20 22:18:17 pavlin Exp $

#ifndef __RIB_DUMMY_RIB_MANAGER_HH__
#define __RIB_DUMMY_RIB_MANAGER_HH__

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"

#include "libproto/proto_state.hh"

#include "libxipc/xrl_std_router.hh"

#include "rib.hh"
#include "rib_client.hh"
#include "register_server.hh"
#include "vifmanager.hh"
#include "xrl_target.hh"


/**
 * @short A dummy RIB manager.
 */
class RibManager : public ProtoState {
public:
    /**
     * RibManager constructor
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
     * @return process status code.
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_vif(const string& vifname, string& err);

    /**
     * Set the vif flags of a configured vif.
     * 
     * @param vifname the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_flags(const string& vifname,
			      bool is_p2p,
			      bool is_loopback,
			      bool is_multicast,
			      bool is_broadcast,
			      bool is_up,
			      string& err);

    /**
     * add_vif_address is called to inform all the RIBs that a new IPv4
     * address has been added to a virtual interface.
     *
     * @param vifname the name of the VIF that the address was added to.
     * @param addr the new address.
     * @param subnet the subnet (masked address) that the new address
     * resides on.
     * @param broadcast the broadcast address to add.
     * @param peer the peer address to add.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_vif_address(const string& vifname,
			const IPv4& addr,
			const IPv4Net& subnet,
			const IPv4& broadcast_addr,
			const IPv4& peer_addr,
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
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
     * @param subnet the subnet (masked address) that the new address
     * resides on.
     * @param peer the peer address to add.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_vif_address(const string& vifname,
			const IPv6& addr,
			const IPv6Net& subnet,
			const IPv6& peer,
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
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
    RibClient* find_rib_client(const string& target_name, int family,
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
     * Make some errors we'd normally mask fatal.  Should be used for
     * testing only.
     */
    void make_errors_fatal();

    /**
     * Register Interest in an XRL target so we can monitor process
     * births and deaths and clean up appropriately when things die.
     *
     * @param target_class the XRL Target Class we're interested in.
     */
    void register_interest_in_target(const string& target_class);

    /**
     * Called in response to registering interest in an XRL target
     *
     * @param e XRL Response code.
     */
    void register_interest_in_target_done(const XrlError& e);

    /**
     * Target Death is called when an XRL target that we've registered
     * an interest in dies.
     *
     * @param target_class the XRL Class of the target that died.
     * @param target_instance the XRL Class Instance of the target that died.
     */
    void target_death(const string& target_class,
		      const string& target_instance);

    /**
     * Add Route Redistributor that generates updates with redist4
     * XRL interface.
     *
     * @param target_name XRL target to receive redistributed routes.
     * @param from_protocol protocol routes are redistributed from.
     * @param unicast apply to unicast rib.
     * @param multicast apply to multicast rib.
     * @param cookie cookie passed in route redistribution XRLs.
     * @param is_xrl_transaction_output if true the add/delete route XRLs
     * are grouped into transactions.
     *
     * @return XORP_OK on success, XORP_ERROR on failure.
     */
    int add_redist_xrl_output4(const string&	target_name,
			       const string&	from_protocol,
			       bool	   	unicast,
			       bool		multicast,
			       const string&	cookie,
			       bool		is_xrl_transaction_output);

    /**
     * Add Route Redistributor that generates updates with redist6
     * XRL interface.
     *
     * @param target_name XRL target to receive redistributed routes.
     * @param from_protocol protocol routes are redistributed from.
     * @param unicast apply to unicast rib.
     * @param multicast apply to multicast rib.
     * @param cookie cookie passed in route redistribution XRLs.
     * @param is_xrl_transaction_output if true the add/delete route XRLs
     * are grouped into transactions.
     *
     * @return XORP_OK on success, XORP_ERROR on failure.
     */
    int add_redist_xrl_output6(const string&	target_name,
			       const string&	from_protocol,
			       bool	   	unicast,
			       bool		multicast,
			       const string&	cookie,
			       bool		is_xrl_transaction_output);

    /**
     * Remove Route Redistributor that generates updates with redist4
     * XRL interface.
     *
     * @param target_name XRL target to receive redistributed routes.
     * @param from_protocol protocol routes are redistributed from.
     * @param unicast apply to unicast rib.
     * @param multicast apply to multicast rib.
     * @param cookie cookie passed in route redistribution XRLs.
     * @param is_xrl_transaction_output if true the add/delete route XRLs
     * are grouped into transactions.
     *
     * @return XORP_OK on success, XORP_ERROR on failure.
     */
    int delete_redist_xrl_output4(const string&	target_name,
				  const string&	from_protocol,
				  bool	   	unicast,
				  bool		multicast,
				  const string&	cookie,
				  bool		is_xrl_transaction_output);

    /**
     * Remove Route Redistributor that generates updates with redist6
     * XRL interface.
     *
     * @param target_name XRL target to receive redistributed routes.
     * @param from_protocol protocol routes are redistributed from.
     * @param unicast apply to unicast rib.
     * @param multicast apply to multicast rib.
     * @param cookie cookie passed in route redistribution XRLs.
     * @param is_xrl_transaction_output if true the add/delete route XRLs
     * are grouped into transactions.
     *
     * @return XORP_OK on success, XORP_ERROR on failure.
     */
    int delete_redist_xrl_output6(const string&	target_name,
				  const string&	from_protocol,
				  bool	   	unicast,
				  bool		multicast,
				  const string&	cookie,
				  bool		is_xrl_transaction_output);

private:
};

#endif // __RIB_DUMMY_RIB_MANAGER_HH__
