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

#ident "$XORP: xorp/rib/rib_client.cc,v 1.6 2003/04/22 19:20:24 mjh Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "rib_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"
#include "xrl/interfaces/fti_xif.hh"

#include "rib_client.hh"

// For the time being this code is synchronous.  Instead of making the
// FTI of the RIB clients support synchronous and asynchronous interfaces, we
// use asynchronous operations here, but make them look asynchronous.  This
// mode of operation may be bad for some FTI implementations.  For the time
// being we are ignoring this issue, but it needs revisiting.
//
// A net result of this mismatch is we end up queuing XRL's here and
// queuing transaction operations in the FTI.  Not a biggie, but it
// makes this end harder to comprehend.

// -------------------------------------------------------------------------
// Synchronous Xrl code

/**
 * Base class for synchronous FTI interaction.  Handles start and commit
 * XRL's for FTI command, eg add_route4, delete_route4, etc.  Derived
 * classes must implement send_command() and an Xrl callback handler
 * for the Xrl dispatch.  This Xrl callback handler should just call
 * SyncFtiCommand::complete().  See AddRoute4 for an example.
 */
class SyncFtiCommand {
public:
    // Callback type of callback to be invoked when synchronous command
    // completes
    typedef ref_ptr<XorpCallback1<void, bool> > CompletionCallback;
    // Callback to get the next add or delete if available.
    typedef ref_ptr<XorpCallback0<SyncFtiCommand *> > GetNextCallback;

    typedef ref_ptr<XorpCallback1<void, const XrlError&> > CommandCompleteCallback;

    SyncFtiCommand(XrlRouter& rtr, const string& target_name)
	: _eventloop(rtr.eventloop()),
	  _fticlient(&rtr), _target_name(target_name), 
	_previously_successful(false) {}
    virtual ~SyncFtiCommand() {}

    void start(const CompletionCallback& cb, const GetNextCallback& gncb);

    // Methods to be implemented by actual commands
    virtual void send_command(uint32_t tid, const CommandCompleteCallback& cb)
	= 0;

protected:
    void redo_start();
    void start_complete(const XrlError& e, const uint32_t *tid);

    void command_complete(const XrlError& e);

    void commit();

    void commit_complete(const XrlError& e);

    SyncFtiCommand *get_next() {
	return _get_next_cb->dispatch();
    }

    void done(bool fatal_error) {
	_completion_cb->dispatch(fatal_error);
    }

    uint32_t tid() const { return _tid; }

    EventLoop& _eventloop;
    XrlFtiV0p2Client	_fticlient;
    const string&	_target_name;

private:
    uint32_t		_tid;		// transaction id
    CompletionCallback	_completion_cb;	// callback called when done
    GetNextCallback	_get_next_cb;	// get the next operation

    XorpTimer _rtx_timer; //Used to re-send after a delay when we
                          //suffered a full transmit buffer.
    bool _previously_successful; //true when the interface has previously 
                                 //succeeded
};

void
SyncFtiCommand::start(const CompletionCallback& ccb,
		      const GetNextCallback& gncb)
{
    _completion_cb = ccb;
    _get_next_cb = gncb;
    if (!_fticlient.
	send_start_transaction(_target_name.c_str(),
			       callback(this,
					&SyncFtiCommand::start_complete))) {
	//The send failed - not enough buffer space?.
	//We resend after a delay
	_rtx_timer 
	    = _eventloop.new_oneoff_after_ms(1000, 
                   callback(this, &SyncFtiCommand::redo_start));
    }
}

void
SyncFtiCommand::redo_start()
{
    start(_completion_cb, _get_next_cb);
}

void
SyncFtiCommand::start_complete(const XrlError& e, const uint32_t *tid)
{
    debug_msg("start completed\n");
    if (e == XrlError::OKAY()) {
	_previously_successful = true;
	_tid = *tid;
	send_command(_tid, callback(this, &SyncFtiCommand::command_complete));
	return;
    } else {
	bool fatal_error = false;
	if ((e == XrlError::NO_FINDER())
	    || (e == XrlError::SEND_FAILED())
	    || (e == XrlError::NO_SUCH_METHOD())
	    || (e == XrlError::RESOLVE_FAILED() && _previously_successful)) {
	    XLOG_ERROR("Fatal transport error from RIB to %s\n",
		       _target_name.c_str());
	    fatal_error = true;
	} else if ((e == XrlError::RESOLVE_FAILED())
		   && (!_previously_successful)) {
	    //We've not yet succeeded in talking to the target, so
	    //give them a chance to get started - not strictly necessary.
	    _rtx_timer 
		= _eventloop.new_oneoff_after_ms(1000, 
                       callback(this, &SyncFtiCommand::redo_start));
	} else {
	    XLOG_ERROR("Could not start Synchronous RibClient command: %s",
		       e.str().c_str());
	}
	done(fatal_error);
    }
}

void
SyncFtiCommand::command_complete(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	if (e == XrlError::NO_FINDER() 
	    || e == XrlError::SEND_FAILED()
	    || e == XrlError::RESOLVE_FAILED()
	    || e == XrlError::NO_SUCH_METHOD()) {
	    XLOG_ERROR("Fatal error while performing RibClient command: %s.",
		       e.str().c_str());
	    //This error is a fatal transport error.
	    done(true);
	    return;
	}

	//XXX something bad happened - not sure how to recover from this.
	XLOG_WARNING("Command failed: %s", e.str().c_str());
	commit();
	return;
    }

    //
    // More commands may have arrived lets check.
    //
    SyncFtiCommand *task = get_next();
    if (task != NULL) {
	task->send_command(_tid,
			   callback(this, &SyncFtiCommand::command_complete));
	return;
    }

    commit();
}

void
SyncFtiCommand::commit()
{
    debug_msg("commit\n");
    _fticlient.send_commit_transaction(_target_name.c_str(),
				       _tid,
				       callback(
					   this,
					   &SyncFtiCommand::commit_complete));
    debug_msg("Sending Commit\n");
}

void
SyncFtiCommand::commit_complete(const XrlError& e)
{
    debug_msg("Commit completed\n");
    bool fatal_error = false;
    if (e != XrlError::OKAY()) {
	if (e == XrlError::NO_FINDER() 
	    || e == XrlError::SEND_FAILED()
	    || e == XrlError::RESOLVE_FAILED()
	    || e == XrlError::NO_SUCH_METHOD()) {
	    XLOG_ERROR("Fatal error while performing RibClient command: %s.",
		       e.str().c_str());
	    fatal_error = true;
	} else {
	    //XXX should handle other errors here too.
	    XLOG_ERROR("Could not commit Synchronous RibClient command: %s.",
		       e.str().c_str());
	}
    }
    done(fatal_error);
}

// -------------------------------------------------------------------------
// Synchronous AddRoute4 command

class AddRoute4 : public SyncFtiCommand {
public:
    AddRoute4(XrlRouter&     rtr,
	      const IPv4Net& dest,
	      const IPv4&    gw,
	      const string&  ifname,
	      const string&  vifname,
	      uint32_t	     metric,
	      uint32_t	     admin_distance,
	      const string&  protocol_origin,
	      const string&  target_name)
	: SyncFtiCommand(rtr, target_name), _dest(dest), _gw(gw),
	  _ifname(ifname), _vifname(vifname),
	  _metric(metric), _admin_distance(admin_distance),
	  _protocol_origin(protocol_origin)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_add_entry4(_target_name.c_str(), tid, _dest, _gw,
				   _ifname, _vifname, _metric, _admin_distance,
				   _protocol_origin, cb);
    }

private:
    IPv4Net	_dest;
    IPv4	_gw;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    uint32_t	_admin_distance;
    string	_protocol_origin;
};

// -------------------------------------------------------------------------
// Synchronous DeleteRoute4 command

class DeleteRoute4 : public SyncFtiCommand {
public:
    DeleteRoute4(XrlRouter& rtr, const IPv4Net& dest,
		 const string& target_name)
	: SyncFtiCommand(rtr, target_name), _dest(dest)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_delete_entry4(_target_name.c_str(), tid, _dest, cb);
    }

private:
    IPv4Net _dest;
};

// -------------------------------------------------------------------------
// Synchronous AddRoute6 command

class AddRoute6 : public SyncFtiCommand {
public:
    AddRoute6(XrlRouter&     rtr,
	      const IPv6Net& dest,
	      const IPv6&    gw,
	      const string&  ifname,
	      const string&  vifname,
	      uint32_t	     metric,
	      uint32_t	     admin_distance,
	      const string&  protocol_origin,
	      const string&  target_name)
	: SyncFtiCommand(rtr, target_name), _dest(dest), _gw(gw),
	  _ifname(ifname), _vifname(vifname),
	  _metric(metric), _admin_distance(admin_distance),
	  _protocol_origin(protocol_origin)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_add_entry6(_target_name.c_str(), tid, _dest, _gw,
				   _ifname, _vifname, _metric, _admin_distance,
				   _protocol_origin, cb);
    }

private:
    IPv6Net	_dest;
    IPv6	_gw;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    uint32_t	_admin_distance;
    string	_protocol_origin;
};

// -------------------------------------------------------------------------
// Synchronous DeleteRoute6 command

class DeleteRoute6 : public SyncFtiCommand {
public:
    DeleteRoute6(XrlRouter& rtr, const IPv6Net& dest,
		 const string& target_name)
	: SyncFtiCommand(rtr, target_name), _dest(dest)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_delete_entry6(_target_name.c_str(), tid, _dest, cb);
    }

private:
    IPv6Net _dest;
};

// ----------------------------------------------------------------------
// Utilities to extract destination, gateway, and vifname from a route entry.

template<typename A>
inline const IPNet<A>&
dest(const IPRouteEntry<A>& re)
{
    return re.net();
}

template<typename A>
inline const A&
gw(const IPRouteEntry<A>& re)
{
    IPNextHop<A> *nh = reinterpret_cast<IPNextHop<A> *>(re.nexthop());
    return nh->addr();
}

template<typename A>
inline const string&
vifname(const IPRouteEntry<A>& re)
{
    return re.vif()->name();
}

template<typename A>
inline const string&
ifname(const IPRouteEntry<A>& re)
{
    return re.vif()->ifname();
}

// -------------------------------------------------------------------------
// RibClient

RibClient::RibClient(XrlRouter& rtr, const string& target_name, size_t max_ops)
    : _xrl_router(rtr),
      _target_name(target_name),
      _busy(false),
      _max_ops(max_ops),
      _op_count(0),
      _enabled(true),
      _failed(false)
{
}

RibClient::~RibClient()
{
}

void
RibClient::add_route(const IPv4Net& dest,
		     const IPv4&    gw,
		     const string&  ifname,
		     const string&  vifname,
		     uint32_t	    metric,
		     uint32_t	    admin_distance,
		     const string&  protocol_origin)
{
    if (_failed) 
	return;
    _tasks.push_back(RibClientTask(new AddRoute4(_xrl_router, dest, gw, 
						 ifname, vifname,
						 metric, admin_distance,
						 protocol_origin,
						 _target_name)));
    start();
}

void
RibClient::delete_route(const IPv4Net& dest)
{
    if (_failed) 
	return;
    _tasks.push_back(RibClientTask(new DeleteRoute4(_xrl_router, dest,
						    _target_name)));
    start();
}

void
RibClient::add_route(const IPv6Net& dest,
		     const IPv6&    gw,
		     const string&  ifname,
		     const string&  vifname,
		     uint32_t	    metric,
		     uint32_t	    admin_distance,
		     const string&  protocol_origin)
{
    if (_failed) 
	return;
    _tasks.push_back(RibClientTask(new AddRoute6(_xrl_router, dest, gw, 
						 ifname, vifname,
						 metric, admin_distance,
						 protocol_origin,
						 _target_name)));
    start();
}

void
RibClient::delete_route(const IPv6Net& dest)
{
    if (_failed) 
	return;
    _tasks.push_back(RibClientTask(new DeleteRoute6(_xrl_router, dest,
						    _target_name)));
    start();
}

void
RibClient::add_route(const IPv4RouteEntry& re)
{
    if (_failed) 
	return;
    add_route(dest(re), gw(re), ifname(re), vifname(re), re.metric(),
	      re.admin_distance(), re.protocol().name());
}

void
RibClient::delete_route(const IPv4RouteEntry& re)
{
    if (_failed) 
	return;
    delete_route(dest(re));
}

void
RibClient::add_route(const IPv6RouteEntry& re)
{
    if (_failed) 
	return;
    add_route(dest(re), gw(re), ifname(re), vifname(re), re.metric(),
	      re.admin_distance(), re.protocol().name());
}

void
RibClient::delete_route(const IPv6RouteEntry& re)
{
    if (_failed) 
	return;
    delete_route(dest(re));
}

size_t
RibClient::tasks_count() const
{
    return _tasks.size();
}

bool
RibClient::tasks_pending() const
{
    XLOG_ASSERT(_tasks.empty());
    return !_tasks.empty();
}

SyncFtiCommand *
RibClient::get_next()
{
    debug_msg("Get next task count %u op count %u\n", (uint32_t)tasks_count(),
	      (uint32_t)_op_count);

    XLOG_ASSERT(!_tasks.empty());

    _completed_tasks.push_back(_tasks.front());
    _tasks.pop_front();
    if (_tasks.empty() || ++_op_count >= _max_ops)
	return NULL;

    debug_msg("Get next found one\n");

    RibClientTask rib_client_task = _tasks.front();

    return &(*rib_client_task);
}

void
RibClient::transaction_completed(bool fatal_error)
{
    XLOG_ASSERT(_busy);

    _busy = false;
    if (fatal_error)
	_failed = true;

    while (!_completed_tasks.empty())
	_completed_tasks.pop_front();
    
    if (!_tasks.empty())
	start();
}

void
RibClient::start()
{
    if (_busy) {
	debug_msg("start: busy\n");
	return;
    }

    if (_tasks.empty()) {
	debug_msg("start: empty\n");
	return;
    }

    if (_enabled) {
	debug_msg("start\n");
	_busy = true;
	_op_count = 0;
	_tasks.front()->start(callback(this,
				       &RibClient::transaction_completed),
			      callback(this, &RibClient::get_next));
    } else {
	_tasks.clear();
    }
}
