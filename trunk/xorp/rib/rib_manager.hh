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

// $XORP: xorp/rib/rib_manager.hh,v 1.27 2004/07/24 01:01:52 pavlin Exp $

#ifndef __RIB_RIB_MANAGER_HH__
#define __RIB_RIB_MANAGER_HH__

#include "libxorp/xorp.h"
#include "libxorp/status_codes.h"

#include "libproto/proto_state.hh"

#include "libxipc/xrl_std_router.hh"

#include "rib.hh"
#include "register_server.hh"
#include "vifmanager.hh"
#include "xrl_target.hh"


class EventLoop;

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
     * @param fea_target the FEA XRL target name.
     */
    RibManager(EventLoop& eventloop, XrlStdRouter& xrl_std_router,
	       const string& fea_target);

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
     * @return the process status code.
     * @see ProcessStatus.
     */
    ProcessStatus status(string& reason) const
    {
	reason = _status_reason;
	return _status_code;
    }

    /**
     * new_vif is called to inform all the RIBs that a new virtual
     * interface has been created.
     *
     * @param vifname the name of the new VIF.
     * @param vif the Vif class instance holding information about the
     * new VIF.
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


    XrlStdRouter& xrl_router() {    return _xrl_router; }

    /**
     * Push the connected routes through the policy filter for re-filtering.
     */
    void push_routes();

    /**
     * Configure a policy filter.
     *
     * Will throw an exception on error.
     *
     * In this case the source match filter of connected routes will be
     * configured.
     *
     * @param filter Identifier of filter to configure.
     * @param conf Configuration of the filter.
     */
    void configure_filter(const uint32_t& filter, const string& conf);

    /**
     * Reset a policy filter.
     *
     * @param filter Identifier of filter to reset.
     */
    void reset_filter(const uint32_t& filter);

    /**
     * @return the global instance of policy filters.
     */
    PolicyFilters&	policy_filters()    { return _policy_filters; }

    /**
     * @return the global instance of the policy redistribution map.
     */
    PolicyRedistMap&	policy_redist_map() { return _policy_redist_map; }


    /**
     * Insert [old ones are kept] policy-tags for a protocol.
     *
     * All routes which contain at least one of these tags, will be
     * redistributed to the protocol.
     *
     * @param protocol Destination protocol for redistribution.
     * @param tags Policy-tags of routes which need to be redistributed.
     */
    void insert_policy_redist_tags(const string& protocol, 
				   const PolicyTags& tags);

    /**
     * Reset the policy redistribution map.
     *
     * This map allows policy redistribution to occur by directing routes with
     * specific policy-tags to specific protocols.
     *
     */
    void reset_policy_redist_tags();

private:
    ProcessStatus       _status_code;
    string              _status_reason;
    EventLoop&		_eventloop;		// The event loop to use
    XrlStdRouter&	_xrl_router;		// The XRL router to use
    RegisterServer	_register_server;    // To notify clients about route change

    RIB<IPv4>		_urib4;			// The IPv4 unicast RIB
    RIB<IPv4>		_mrib4;			// The IPv4 multicast RIB
    RIB<IPv6>		_urib6;			// The IPv6 unicast RIB
    RIB<IPv6>		_mrib6;			// The IPv6 multicast RIB

    VifManager		_vif_manager;		// The VIF manager
    XrlRibTarget	_xrl_rib_target;
    set<string>		_targets_of_interest;	// Monitored XRL targets
    XorpTimer		_status_update_timer;	// Timer for periodic checks of RIB status

    const string	_fea_target;

    PolicyFilters	_policy_filters;	// Connected routes filter.
						// only one should be present.

    PolicyRedistMap	_policy_redist_map;	// Policy registribution map
						// [links policy-tags to
						// targets].
						// only one should be present.
};

#endif // __RIB_RIB_MANAGER_HH__
