// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_packet_acl.hh,v 1.7 2007/05/23 12:12:35 pavlin Exp $

#ifndef __FEA_XRL_PACKET_ACL_HH__
#define __FEA_XRL_PACKET_ACL_HH__

#include <map>

#include "libxorp/xlog.h"
#include "libxipc/xrl_cmd_map.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_transaction.hh"
#include "xrl/targets/packet_acl_base.hh"


/**
 * Helper class for helping with packet ACL configuration transactions
 * via an Xrl interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the PaTransactionManager class.
 */
class XrlPacketAclTarget : public XrlStdRouter,
			   public XrlPacketAclTargetBase {
public:
    enum { BROWSE_TIMEOUT_MS = 15000 };

    /**
     * Constructor.
     *
     * @param eventloop an EventLoop which will be used for scheduling timers.
     * @param class_name the class name for this service.
     * @param finder_hostname the Finder host name.
     * @param finder_port the Finder port.
     * @param pat a PaTransactionManager which manages accesses to the
     *		  underlying ACL tables.
     */
    XrlPacketAclTarget(EventLoop&	eventloop,
		       const string&	class_name,
		       const string&	finder_hostname,
		       uint16_t		finder_port,
		       PaTransactionManager& pat,
		       uint32_t		browse_timeout_ms = BROWSE_TIMEOUT_MS);

    /**
     * Destructor.
     *
     * Dissociates instance commands from command map.
     */
     virtual ~XrlPacketAclTarget();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop& eventloop() { return (_eventloop); }

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

    /**
     * Get a const reference to the XrlRouter instance.
     *
     * @return a const reference to the XrlRouter (@ref XrlRouter) instance.
     */
    const XrlRouter& xrl_router() const { return *this; }

    /**
     * Get the browse timeout (in milliseconds).
     *
     * @return the browse timeout (in milliseconds).
     */
     uint32_t browse_timeout_ms() const { return _browse_timeout_ms; }

protected:
    /* ---------------------------------------------------------------- */
    /* XRL common interface methods */

    /**
     *  Function that needs to be implemented to:
     *
     *  Get name of Xrl Target
     */
     XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get version string from Xrl Target
     */
     XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get status of Xrl Target
     */
     XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Function that needs to be implemented to:
     *
     *  Request clean shutdown of Xrl Target
     */
     XrlCmdError common_0_1_shutdown();

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: global methods */

    /**
     *  Function that needs to be implemented to:
     *  Get the name of the ACL back-end provider currently in use.
     */
     XrlCmdError packet_acl_0_1_get_backend(
	// Output values,
	string&	name);

    /**
     *  Function that needs to be implemented to:
     *  Set the underlying packet ACL provider type in use. NOTE: If XORP rules
     *  currently exist, this operation will perform an implicit flush and
     *  reload when switching to the new provider.
     */
     XrlCmdError packet_acl_0_1_set_backend(
	// Input values,
	const string&	name);

    /**
     *  Function that needs to be implemented to:
     *  Get the underlying packet ACL provider version in use.
     */
     XrlCmdError packet_acl_0_1_get_version(
	// Output values,
	string&	version);

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: transaction control methods */

    /**
     *  Function that needs to be implemented to:
     *  Start an ACL configuration transaction.
     *
     *  @param tid The number of the newly started transaction.
     */
    XrlCmdError packet_acl_0_1_start_transaction(
	// Output values,
	uint32_t& tid);

    /**
     *  Function that needs to be implemented to:
     *  Commit a previously started ACL configuration transaction.
     *
     *  @param tid The number of the transaction to commit.
     */
    XrlCmdError packet_acl_0_1_commit_transaction(
	// Input values,
	const uint32_t& tid);

    /**
     *  Function that needs to be implemented to:
     *  Abort an ACL configuration transaction in progress.
     *
     *  @param tid The number of the transaction to abort.
     */
    XrlCmdError packet_acl_0_1_abort_transaction(
	// Input values,
	const uint32_t& tid);

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: per-transaction methods */

    /**
     *  Function that needs to be implemented to:
     *  Add an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be applied.
     *  @param vifname Name of the vif where this filter is to be applied.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *  @param action Action to take when this ACL entry is matched.
     */
    XrlCmdError packet_acl_0_1_add_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *  Delete an IPv4 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be deleted.
     *  @param vifname Name of the vif where this filter is to be deleted.
     *  @param src Source IPv4 address with network prefix.
     *  @param dst Destination IPv4 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError packet_acl_0_1_delete_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *  Delete all IPv4 family ACL entries.
     *
     *  @param tid The number of the transaction for this operation.
     */
     XrlCmdError packet_acl_0_1_delete_all_entries4(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError packet_acl_0_1_get_entry_list_start4(
	// Output values,
	uint32_t&	token,
	bool&		more);

    XrlCmdError packet_acl_0_1_get_entry_list_next4(
	// Input values,
	const uint32_t&	token,
	// Output values,
	string&		ifname,
	string&		vifname,
	IPv4Net&	src,
	IPv4Net&	dst,
	uint32_t&	proto,
	uint32_t&	sport,
	uint32_t&	dport,
	string&		action,
	bool&		more);

    /**
     *  Function that needs to be implemented to:
     *  Add an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be applied.
     *  @param vifname Name of the vif where this filter is to be applied.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *  @param action Action to take when this filter is matched.
     */
     XrlCmdError packet_acl_0_1_add_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *  Delete an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be deleted.
     *  @param vifname Name of the vif where this filter is to be deleted.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError packet_acl_0_1_delete_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *  Delete all IPv6 family ACL entries.
     *
     *  @param tid The number of the transaction for this operation.
     */
     XrlCmdError packet_acl_0_1_delete_all_entries6(
	// Input values,
	const uint32_t&	tid);

protected:
    /**
     * Used to hold state for clients reading snapshots of the ACL tables.
     */
    struct PaBrowseState {
    public:
	PaBrowseState(const XrlPacketAclTarget& xpa,
		      const XorpTimer& timeout_timer,
		      const PaSnapshot4* snap4,
		      size_t idx = 0);
	PaBrowseState(const XrlPacketAclTarget& xpa,
		      const PaSnapshot4* snap4,
		      size_t idx = 0);
	~PaBrowseState();
	size_t index() const			{ return _idx; }
	void set_index(size_t idx)		{ _idx = idx; }
	const PaSnapshot4* snap4() const	{ return _snap4; }
	void defer_timeout();
	void cancel_timeout();
    private:
	const XrlPacketAclTarget&	_xpa;
	XorpTimer			_timeout_timer;
	const PaSnapshot4*		_snap4;	// XXX: not refcnted!
	size_t				_idx;
    };

    typedef map<uint32_t, PaBrowseState> PaBrowseDB;

    void crank_token();
    void timeout_browse(uint32_t token);

    EventLoop&		_eventloop;
    PaTransactionManager& _pat;
    uint32_t		_browse_timeout_ms;

    uint32_t		_next_token;
    PaBrowseDB		_bdb;
    bool		_is_running;	// True if the service is running
};

#endif /* __FEA_XRL_PACKET_ACL_HH__ */
