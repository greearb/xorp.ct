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

#ident "$XORP: xorp/rib/fea_client.cc,v 1.21 2002/12/10 06:56:08 mjh Exp $"

#include "config.h"
#include "urib_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ref_ptr.hh"
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

/* ------------------------------------------------------------------------- */
/* Synchronous Xrl code */

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

    SyncFtiCommand(XrlRouter& rtr) : _fticlient(&rtr), _started(false) {}
    virtual ~SyncFtiCommand() {}

    void start(const CompletionCallback& cb);
    bool started() const { return _started; }

    // Methods to be implemented by actual commands
    virtual void send_command() = 0;

protected:
    void start_complete(const XrlError& e, const uint32_t* tid);

    void commit();

    void commit_complete(const XrlError& e);

    void done() {
	_completion_cb->dispatch();
    }

protected:
    uint32_t tid() const { return _tid; }

protected:
    XrlFtiV0p1Client	_fticlient;

private:
    bool		_started;	// start has been called
    uint32_t		_tid;		// transaction id
    CompletionCallback	_completion_cb;	// callback called when done
};

void
SyncFtiCommand::start(const CompletionCallback& cb) {
    _started = true;
    _completion_cb = cb;
    _fticlient.send_start_transaction(
	server, callback(this, &SyncFtiCommand::start_complete)
	);
};

void
SyncFtiCommand::start_complete(const XrlError& e, const uint32_t* tid)
{
    debug_msg("start completed\n");
    if (e == XrlError::OKAY()) {
	_tid = *tid;
	send_command();
	return;
    }
    XLOG_ERROR("Could not start Synchronous FEA command: %s.",
	       e.str().c_str());
    done();
}

void
SyncFtiCommand::commit() {
    _fticlient.send_commit_transaction(
	server, _tid, callback(this, &SyncFtiCommand::commit_complete)
	);
    debug_msg("Sending Commit\n");
}

void SyncFtiCommand::commit_complete(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not commit Synchronous FEA command: %s.",
		   e.str().c_str());
    }
    done();
    debug_msg("Commit completed\n");
}

/* ------------------------------------------------------------------------- */
/* Synchronus AddRoute4 command */

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

    void send_command()
    {
	_fticlient.send_add_entry4(server, tid(), _dest, _gw,
				   _ifname, _vifname,
				   callback(this,
					    &AddRoute4::command_completed));
    }

    void command_completed(const XrlError& /* r */)
    {
	commit(); // Signal end of transaction
    }

private:
    IPv4Net _dest;
    IPv4    _gw;
    string  _ifname;
    string  _vifname;
};

/* ------------------------------------------------------------------------- */
/* Synchronus AddRoute4 command */

class DeleteRoute4 : public SyncFtiCommand {
public:
    DeleteRoute4(XrlRouter& rtr, const IPv4Net& dest)
	: SyncFtiCommand(rtr), _dest(dest)
    {}

    void send_command()
    {
	_fticlient.send_delete_entry4(server, tid(), _dest,
				      callback(this,
					    &DeleteRoute4::command_completed));
    }

    void command_completed(const XrlError& /* r */)
    {
	commit(); // Signal end of transaction
    }

private:
    IPv4Net _dest;
};

/* ------------------------------------------------------------------------- */
/* Synchronus AddRoute6 command */

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

    void send_command()
    {
	_fticlient.send_add_entry6(
	    server, tid(), _dest, _gw, _ifname, _vifname,
	    callback(this, &AddRoute6::command_completed)
	    );
    }

    void command_completed(const XrlError& /* r */)
    {
	commit(); // Signal end of transaction
    }

private:
    IPv6Net _dest;
    IPv6    _gw;
    string  _ifname;
    string  _vifname;
};

/* ------------------------------------------------------------------------- */
/* Synchronus AddRoute6 command */

class DeleteRoute6 : public SyncFtiCommand {
public:
    DeleteRoute6(XrlRouter& rtr, const IPv6Net& dest)
	: SyncFtiCommand(rtr), _dest(dest)
    {}

    void send_command()
    {
	_fticlient.send_delete_entry6(server, tid(), _dest,
				      callback(this,
					    &DeleteRoute6::command_completed));
    }

    void command_completed(const XrlError& /* r */)
    {
	commit(); // Signal end of transaction
    }

private:
    IPv6Net _dest;
};

/* ---------------------------------------------------------------------- */
/* Utilities to extract destination, gateway, and vifname from a route
 * entry. */

template<typename A> inline const IPNet<A>&
dest(const IPRouteEntry<A>& re)
{
    return re.net();
}

template<typename A> inline const A&
gw(const IPRouteEntry<A>& re)
{
    IPNextHop<A>* nh = reinterpret_cast<IPNextHop<A>*>(re.nexthop());
    return nh->addr();
}

template<typename A> inline const string&
vifname(const IPRouteEntry<A>& re)
{
    return re.vif()->name();
}

template<typename A> inline const string&
ifname(const IPRouteEntry<A>& re)
{
    return re.vif()->ifname();
}

/* ------------------------------------------------------------------------- */
/* FeaClient */

FeaClient::FeaClient(XrlRouter& rtr)
    : _xrl_router(rtr)
{}

FeaClient::~FeaClient()
{}

void
FeaClient::add_route(const IPv4Net& dest,
		     const IPv4&    gw,
		     const string&  ifname,
		     const string&  vifname)
{
    _tasks.push_back(FeaClientTask(new AddRoute4(_xrl_router, dest, gw, 
						 ifname, vifname)));
    crank();
}

void
FeaClient::delete_route(const IPv4Net& dest)
{
    _tasks.push_back(
	FeaClientTask(new DeleteRoute4(_xrl_router, dest))
	);
    crank();
}

void
FeaClient::add_route(const IPv6Net& dest,
		     const IPv6&    gw,
		     const string&  ifname,
		     const string&  vifname)
{
    _tasks.push_back(FeaClientTask(new AddRoute6(_xrl_router, dest, gw, 
						 ifname, vifname)));
    crank();
}

void
FeaClient::delete_route(const IPv6Net& dest)
{
    _tasks.push_back(FeaClientTask(new DeleteRoute6(_xrl_router, dest)));
    crank();
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
    assert(_tasks.empty() || _tasks.front()->started());
    return !_tasks.empty();
}

void
FeaClient::task_completed()
{
    _tasks.erase(_tasks.begin());
    crank();
}

void
FeaClient::crank()
{
    if (_tasks.empty() || _tasks.front()->started()) {
	// No jobs to do, or job at front of queue is running
	debug_msg("cranking: nothing to do %s\n",
		  _tasks.empty() ? "empty" : "running");
	return;
    }
    debug_msg("cranking\n");
    _tasks.front()->start(callback(this, &FeaClient::task_completed));
}
