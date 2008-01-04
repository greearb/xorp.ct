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

#ident "$XORP: xorp/fea/xrl_packet_acl.cc,v 1.9 2007/09/15 19:52:41 pavlin Exp $"

#include <map>

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/random.h"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"
#include "libxipc/xuid.hh"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_transaction.hh"
#include "pa_backend.hh"

#include "xrl_packet_acl.hh"

static const char* PA_BAD_ID =
"Expired or invalid token or transaction id presented.";
static const char* PA_BAD_PARAM =
"Expired or invalid parameter presented.";
static const char* PA_MAX_OPS_HIT =
"Resource limit on number of operations in a transaction hit.";
static const char* PA_MAX_TRANSACTIONS_HIT =
"Resource limit on number of pending transactions hit.";
static const char* PA_NOT_READY = "Underlying provider is NULL.";

// ----------------------------------------------------------------------------
// XrlPacketAclTarget constructor/destructor

XrlPacketAclTarget::XrlPacketAclTarget(EventLoop& eventloop,
				       const string& class_name,
				       const string& finder_hostname,
				       uint16_t finder_port,
				       PaTransactionManager& pat,
				       uint32_t browse_timeout_ms)
    : XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlPacketAclTargetBase(&xrl_router()),
      _eventloop(eventloop),
      _pat(pat),
      _browse_timeout_ms(browse_timeout_ms),
      _is_running(false)
{
}

XrlPacketAclTarget::~XrlPacketAclTarget()
{
}

int
XrlPacketAclTarget::startup()
{
    _is_running = true;

    return (XORP_OK);
}

int
XrlPacketAclTarget::shutdown()
{
    _is_running = false;

    return (XORP_OK);
}

bool
XrlPacketAclTarget::is_running() const
{
    return (_is_running);
}

// ----------------------------------------------------------------------------
// common/0.1 interface implementation

XrlCmdError
XrlPacketAclTarget::common_0_1_get_target_name(string& name)
{
    name = XrlPacketAclTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPacketAclTarget::common_0_1_get_version(string& version)
{
    version = XORP_MODULE_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPacketAclTarget::common_0_1_get_status(uint32_t& status,
       string& reason)
{
    // TODO: Make this XRL more informative about the state of the
    // firewall subsystem.
    status = PROC_READY;
    reason = "";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPacketAclTarget::common_0_1_shutdown()
{
    //_fw->destroy_provider();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// packet_acl/0.1 interface implementation

/**
 *  Function that needs to be implemented to:
 *  Get the name of the underlying provider in use.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_get_backend(
	// Output values,
	string&	name)
{
    PaBackend* pbackend = _pat.get_backend();
    if (pbackend == NULL)
	return XrlCmdError::COMMAND_FAILED(PA_NOT_READY);

    name = pbackend->get_name();
    return XrlCmdError::OKAY();
}

/**
 *  Function that needs to be implemented to:
 *  Set the underlying provider type in use.
 *
 *  @param provider Name of the provider to use.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_set_backend(
	// Input values,
	const string&	name)
{
    if (_pat.set_backend(name.c_str()) == XORP_OK)
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_NOT_READY);
}

/**
 *  Function that needs to be implemented to:
 *  Get the underlying IP filter provider version in use.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_get_version(
	// Output values,
	string&	version)
{
    PaBackend* pbackend = _pat.get_backend();
    if (pbackend == NULL)
	return XrlCmdError::COMMAND_FAILED(PA_NOT_READY);

    version = pbackend->get_version();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// packet_acl/0.1 transaction control methods

/**
 *  Function that needs to be implemented to:
 *  Start an ACL configuration transaction.
 *
 *  @param tid The number of the newly started transaction.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_start_transaction(
	// Output values,
	uint32_t& tid)
{
    if (_pat.start(tid))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_MAX_TRANSACTIONS_HIT);
}

/**
 *  Function that needs to be implemented to:
 *  Commit a previously started ACL configuration transaction.
 *
 *  @param tid The number of the transaction to commit.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_commit_transaction(
	// Input values,
	const uint32_t& tid)
{
    if (_pat.commit(tid)) {
	const string& errmsg = _pat.error();
	if (errmsg.empty())
	    return XrlCmdError::OKAY();
	return XrlCmdError::COMMAND_FAILED(errmsg);
    }
    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Abort an ACL configuration transaction in progress.
 *
 *  @param tid The number of the transaction to abort.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_abort_transaction(
	// Input values,
	const uint32_t& tid)
{
    if (_pat.abort(tid))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

// ----------------------------------------------------------------------------
// packet_acl/0.1 per-transaction methods

/**
 *  Function that needs to be implemented to:
 *  Add an IPv4 ACL entry.
 *
 *  @param tid The number of the transaction for this operation.
 *  @param ifname Name of the interface where this entry is to be applied.
 *  @param vifname Name of the vif where this entry is to be applied.
 *  @param src Source IPv4 address with network prefix.
 *  @param dst Destination IPv4 address with network prefix.
 *  @param proto IP protocol number for match (0-255, 255 is wildcard).
 *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
 *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
 *  @param action Action to take when this entry is matched.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_add_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action)
{
    PaAction naction = PaActionWrapper::convert(action);
    if (naction == PaAction(ACTION_INVALID))
	return XrlCmdError::COMMAND_FAILED(PA_BAD_PARAM);

    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaAddEntry4(_pat.ptm(),
	    ifname, vifname, src, dst, proto, sport, dport, naction)
    );

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Delete an IPv4 ACL entry.
 *
 *  @param tid The number of the transaction for this operation.
 *  @param ifname Name of the interface where this entry is to be deleted.
 *  @param vifname Name of the vif where this entry is to be deleted.
 *  @param src Source IPv4 address with network prefix.
 *  @param dst Destination IPv4 address with network prefix.
 *  @param proto IP protocol number for match (0-255, 255 is wildcard).
 *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
 *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_delete_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport)
{
    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaDeleteEntry4(_pat.ptm(),
	    ifname, vifname, src, dst, proto, sport, dport)
    );

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Delete all IPv4 ACL entries.
 *
 *  @param tid The number of the transaction for this operation.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_delete_all_entries4(
	// Input values,
	const uint32_t&	tid)
{
    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaDeleteAllEntries4(_pat.ptm()));

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Get a token for a list of IPv4 ACL entries.
 *
 *  @param token to be provided when calling get_entry_list_next4.
 *  @param more true if the list is not empty.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_get_entry_list_start4(
	// Output values,
	uint32_t&	token,
	bool&		more)
{
    // Get a snapshot of top-level ACL state for browsing.
    const PaSnapshot4* snap4 = _pat.ptm().create_snapshot4();

    // Check if it's empty. If it is, we just need to clean up.
    const PaSnapTable4& table4 = snap4->data();
    if (table4.empty()) {
	delete snap4;
	more = false;
	return XrlCmdError::OKAY();
    }

    // Otherwise, allocate a token, and store it in our map of active
    // browsers.  Create a watchdog timer if necessary.
    crank_token();
    token = _next_token;

    if (browse_timeout_ms()) {
	XorpTimer t = _eventloop.new_oneoff_after_ms(
	    browse_timeout_ms(),
	    callback(this, &XrlPacketAclTarget::timeout_browse, token)
	);
	_bdb.insert(PaBrowseDB::value_type(token,
	    PaBrowseState(*this, t, snap4))
	);
    } else {
	_bdb.insert(PaBrowseDB::value_type(token,
	    PaBrowseState(*this, snap4))
	);
    }
    return XrlCmdError::OKAY();
}

/**
 *  Function that needs to be implemented to:
 *  Get the next item in a list of IPv4 ACL entries.
 *
 *  @param token returned by a previous call to get_entry_list_start4.
 *  @param ifname Name of the interface where this filter exists.
 *  @param vifname Name of the vif where this filter exists.
 *  @param src Source IPv6 address with network prefix.
 *  @param dst Destination IPv6 address with network prefix.
 *  @param proto IP protocol number for match (0-255, 255 is wildcard).
 *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
 *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
 *  @param action Action taken when this filter is matched.
 *  @param more true if the list has more items remaining.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_get_entry_list_next4(
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
	bool&		more)
{
    // Check if we were presented with a valid token.
    PaBrowseDB::iterator i = _bdb.find(token);
    if (i == _bdb.end())
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
    PaBrowseState& pbs = i->second;

    const PaSnapTable4& table4 = pbs.snap4()->data();
    size_t index = pbs.index();
    XLOG_ASSERT(index < table4.size());

    // Marshal PaEntry4 into XRL output variables.
    const PaEntry4& entry = table4[index];
    ifname = entry.ifname();
    vifname = entry.vifname();
    src = entry.src();
    dst = entry.dst();
    proto = entry.proto();
    sport = entry.sport();
    dport = entry.dport();
    action = entry.action();

    // Bump index to point to next member.
    pbs.set_index(++index);

    // Check if there are more elements remaining in the snapshot.
    if (index < table4.size()) {
	// If so, defer the timeout, and signal the more-ness condition.
	more = true;
	if (browse_timeout_ms())
	    pbs.defer_timeout();
    } else {
	// If not, cancel the timeout, and signal the no-more-ness condition.
	more = false;
	if (browse_timeout_ms())
	    pbs.cancel_timeout();
	_bdb.erase(i);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Function that needs to be implemented to:
 *  Add an IPv6 ACL entry.
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
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_add_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action)
{
    PaAction naction = PaActionWrapper::convert(action);
    if (naction == PaAction(ACTION_INVALID))
	return XrlCmdError::COMMAND_FAILED(PA_BAD_PARAM);

    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaAddEntry6(_pat.ptm(),
	    ifname, vifname, src, dst, proto, sport, dport, naction)
    );

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Delete an IPv6 ACL entry.
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
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_delete_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport)
{
    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaDeleteEntry6(_pat.ptm(),
	    ifname, vifname, src, dst, proto, sport, dport)
    );

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

/**
 *  Function that needs to be implemented to:
 *  Delete all IPv6 ACL entries.
 *
 *  @param tid The number of the transaction for this operation.
 */
XrlCmdError
XrlPacketAclTarget::packet_acl_0_1_delete_all_entries6(
	// Input values,
	const uint32_t&	tid)
{
    uint32_t n_ops;

    if (_pat.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);

    if (_pat.retrieve_max_ops() <= n_ops)
	return XrlCmdError::COMMAND_FAILED(PA_MAX_OPS_HIT);

    PaTransactionManager::Operation op(
	new PaDeleteAllEntries6(_pat.ptm()));

    if (_pat.add(tid, op))
	return XrlCmdError::OKAY();

    return XrlCmdError::COMMAND_FAILED(PA_BAD_ID);
}

// ----------------------------------------------------------------------------

/*
 * Crank the token ID. Shamelessly stolen from libxorp/transaction.cc.
 */
void
XrlPacketAclTarget::crank_token()
{
    _next_token++;
    do {
	_next_token += (xorp_random() & 0xfffff);
    } while (_bdb.find(_next_token) != _bdb.end());
}

/*
 * Clean up after a timed-out browse client.
 */
void
XrlPacketAclTarget::timeout_browse(uint32_t token)
{
    PaBrowseDB::iterator i = _bdb.find(token);
    if (i == _bdb.end())
	return;
    debug_msg("Timing out token id %u\n", XORP_UINT_CAST(token));
    _bdb.erase(i);
}

// ----------------------------------------------------------------------------
// Inner class to deal with browse state.

XrlPacketAclTarget::PaBrowseState::PaBrowseState(
    const XrlPacketAclTarget& xpa,
    const XorpTimer& timeout_timer,
    const PaSnapshot4* snap4,
    size_t idx)
    : _xpa(xpa), _timeout_timer(timeout_timer), _snap4(snap4), _idx(idx)
{
}

XrlPacketAclTarget::PaBrowseState::PaBrowseState(
    const XrlPacketAclTarget& xpa,
    const PaSnapshot4* snap4,
    size_t idx)
    : _xpa(xpa), _snap4(snap4), _idx(idx)
{
}

XrlPacketAclTarget::PaBrowseState::~PaBrowseState()
{
    if (_snap4 != NULL)
	delete _snap4;
}

void
XrlPacketAclTarget::PaBrowseState::defer_timeout()
{
    uint32_t timeout_ms = _xpa.browse_timeout_ms();
    if (timeout_ms)
	_timeout_timer.schedule_after_ms(timeout_ms);
}

void
XrlPacketAclTarget::PaBrowseState::cancel_timeout()
{
    _timeout_timer.clear();
}

// ----------------------------------------------------------------------------
