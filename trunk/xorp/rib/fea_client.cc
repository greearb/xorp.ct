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

#ident "$XORP: xorp/rib/fea_client.cc,v 1.10 2003/03/17 23:32:42 pavlin Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "rib_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"
#include "xrl/interfaces/fea_fti_xif.hh"

#include "fea_client.hh"

static const char *server = "fea";

// For the time being this code is synchronous.  Instead of making the
// FTI of the FEA support synchronous and asynchronous interfaces, we
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
    typedef ref_ptr<XorpCallback0<void> > CompletionCallback;
    // Callback to get the next add or delete if available.
    typedef ref_ptr<XorpCallback0<SyncFtiCommand *> > GetNextCallback;

    typedef ref_ptr<XorpCallback1<void, const XrlError&> >
    CommandCompleteCallback;

    SyncFtiCommand(XrlRouter& rtr) : _fticlient(&rtr) {}
    virtual ~SyncFtiCommand() {}

    void start(const CompletionCallback& cb, const GetNextCallback& gncb);

    // Methods to be implemented by actual commands
    virtual void send_command(uint32_t tid, const CommandCompleteCallback& cb)
	= 0;

protected:
    void start_complete(const XrlError& e, const uint32_t *tid);

    void command_complete(const XrlError& e);

    void commit();

    void commit_complete(const XrlError& e);

    SyncFtiCommand *get_next() {
	return _get_next_cb->dispatch();
    }

    void done() {
	_completion_cb->dispatch();
    }

protected:
    uint32_t tid() const { return _tid; }

protected:
    XrlFtiV0p1Client	_fticlient;

private:
    uint32_t		_tid;		// transaction id
    GetNextCallback	_get_next_cb;	// Get the next operation
    CompletionCallback	_completion_cb;	// callback called when done
};

void
SyncFtiCommand::start(const CompletionCallback& cb,
		      const GetNextCallback& gncb)
{
    _completion_cb = cb;
    _get_next_cb = gncb;
    _fticlient.send_start_transaction(server,
				      callback(
					  this,
					  &SyncFtiCommand::start_complete));
}

void
SyncFtiCommand::start_complete(const XrlError& e, const uint32_t *tid)
{
    debug_msg("start completed\n");
    if (e == XrlError::OKAY()) {
	_tid = *tid;
	send_command(_tid, callback(this, &SyncFtiCommand::command_complete));
	return;
    }
    XLOG_ERROR("Could not start Synchronous FEA command: %s", e.str().c_str());
    done();
}

void
SyncFtiCommand::command_complete(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
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
    _fticlient.send_commit_transaction(server,
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
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not commit Synchronous FEA command: %s.",
		   e.str().c_str());
    }
    done();
}

// -------------------------------------------------------------------------
// Synchronous AddRoute4 command

class AddRoute4 : public SyncFtiCommand {
public:
    AddRoute4(XrlRouter&     rtr,
	      const IPv4Net& dest,
	      const IPv4&    gw,
	      const string&  ifname,
	      const string&  vifname)
	: SyncFtiCommand(rtr), _dest(dest), _gw(gw),
	  _ifname(ifname), _vifname(vifname)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_add_entry4(server, tid, _dest, _gw,
				   _ifname, _vifname, cb);
    }

private:
    IPv4Net _dest;
    IPv4    _gw;
    string  _ifname;
    string  _vifname;
};

// -------------------------------------------------------------------------
// Synchronous DeleteRoute4 command

class DeleteRoute4 : public SyncFtiCommand {
public:
    DeleteRoute4(XrlRouter& rtr, const IPv4Net& dest)
	: SyncFtiCommand(rtr), _dest(dest)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_delete_entry4(server, tid, _dest, cb);
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
	      const string&  vifname)
	: SyncFtiCommand(rtr), _dest(dest), _gw(gw),
	  _ifname(ifname), _vifname(vifname)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_add_entry6(server, tid, _dest, _gw, _ifname,
				   _vifname, cb);
    }

private:
    IPv6Net _dest;
    IPv6    _gw;
    string  _ifname;
    string  _vifname;
};

// -------------------------------------------------------------------------
// Synchronous DeleteRoute6 command

class DeleteRoute6 : public SyncFtiCommand {
public:
    DeleteRoute6(XrlRouter& rtr, const IPv6Net& dest)
	: SyncFtiCommand(rtr), _dest(dest)
    {}

    void send_command(uint32_t tid, const CommandCompleteCallback& cb) {
	_fticlient.send_delete_entry6(server, tid, _dest, cb);
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
    IPNextHop<A>* nh = reinterpret_cast<IPNextHop<A>*>(re.nexthop());
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
// FeaClient

FeaClient::FeaClient(XrlRouter& rtr, uint32_t	max_ops)
    : _xrl_router(rtr), _busy(false), _max_ops(max_ops), _enabled(true)
{
}

FeaClient::~FeaClient()
{
}

void
FeaClient::set_enabled(bool en)
{
    _enabled = en;
}

bool
FeaClient::enabled() const
{
    return _enabled;
}

void
FeaClient::add_route(const IPv4Net& dest,
		     const IPv4&    gw,
		     const string&  ifname,
		     const string&  vifname)
{
    _tasks.push_back(FeaClientTask(new AddRoute4(_xrl_router, dest, gw, 
						 ifname, vifname)));
    start();
}

void
FeaClient::delete_route(const IPv4Net& dest)
{
    _tasks.push_back(FeaClientTask(new DeleteRoute4(_xrl_router, dest)));
    start();
}

void
FeaClient::add_route(const IPv6Net& dest,
		     const IPv6&    gw,
		     const string&  ifname,
		     const string&  vifname)
{
    _tasks.push_back(FeaClientTask(new AddRoute6(_xrl_router, dest, gw, 
						 ifname, vifname)));
    start();
}

void
FeaClient::delete_route(const IPv6Net& dest)
{
    _tasks.push_back(FeaClientTask(new DeleteRoute6(_xrl_router, dest)));
    start();
}

void
FeaClient::add_route(const IPv4RouteEntry& re)
{
    add_route(dest(re), gw(re), ifname(re), vifname(re));
}

void
FeaClient::delete_route(const IPv4RouteEntry& re)
{
    delete_route(dest(re));
}

void
FeaClient::add_route(const IPv6RouteEntry& re)
{
    add_route(dest(re), gw(re), ifname(re), vifname(re));
}

void
FeaClient::delete_route(const IPv6RouteEntry& re)
{
    delete_route(dest(re));
}

size_t
FeaClient::tasks_count() const
{
    return _tasks.size();
}

bool
FeaClient::tasks_pending() const
{
    XLOG_ASSERT(_tasks.empty());
    return !_tasks.empty();
}

SyncFtiCommand *
FeaClient::get_next()
{
    debug_msg("Get next task count %u op count %d\n", (uint32_t)tasks_count(),
	      _op_count);

    XLOG_ASSERT(!_tasks.empty());

    _completed_tasks.push_back(_tasks.front());
    _tasks.erase(_tasks.begin());
    if (_tasks.empty() || ++_op_count >= _max_ops)
	return NULL;

    debug_msg("Get next found one\n");

    FeaClientTask fct = _tasks.front();

    return &(*fct);
}

void
FeaClient::transaction_completed()
{
    XLOG_ASSERT(_busy);

    _busy = false;

    while (!_completed_tasks.empty())
	_completed_tasks.erase(_completed_tasks.begin());
	
    if (!_tasks.empty())
	start();
}

void
FeaClient::start()
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
				       &FeaClient::transaction_completed),
			      callback(this, &FeaClient::get_next));
    } else {
	_tasks.clear();
    }
}
