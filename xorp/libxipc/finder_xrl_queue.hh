// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/finder_xrl_queue.hh,v 1.17 2008/10/02 21:57:21 bms Exp $

#ifndef __LIBXIPC_FINDER_XRL_QUEUE_HH__
#define __LIBXIPC_FINDER_XRL_QUEUE_HH__


#include "libxorp/ref_ptr.hh"

class FinderXrlCommandBase;

/**
 * @short Xrl Queue for Finder.
 *
 * The FinderXrlCommandQueue holds and dispatches Xrls for the Finder.
 * Commands are added with the @ref FinderXrlCommandQueue::enqueue
 * method and are serially dispatched without any additional
 * intervention.  During the completion of each Xrl in the queue triggers
 * the sending of the next Xrl.
 */
class FinderXrlCommandQueue
{
public:
    typedef ref_ptr<FinderXrlCommandBase> Command;

public:
    FinderXrlCommandQueue(FinderMessengerBase* messenger);
    FinderXrlCommandQueue(const FinderXrlCommandQueue& oq);
    ~FinderXrlCommandQueue();

    FinderMessengerBase& messenger()	{ return *_m; }

    void enqueue(const Command& cmd);

protected:
    void push();
    void dispatch_one();
    EventLoop& eventloop();

protected:
    friend class FinderXrlCommandBase;
    void crank();
    void kill_messenger();

private:
    FinderXrlCommandQueue& operator=(const FinderXrlCommandQueue&); // no impl

private:
    FinderMessengerBase* _m;
    list<Command>	 _cmds;
    bool		 _pending;
    XorpTimer		 _dispatcher;
};

/**
 * @short Base class for Xrls sent from Finder.
 */
class FinderXrlCommandBase
{
public:
    FinderXrlCommandBase(FinderXrlCommandQueue& q) : _queue(q) {}
    virtual ~FinderXrlCommandBase() {}

    FinderXrlCommandQueue& queue()	{ return _queue; }
    FinderMessengerBase& messenger()	{ return _queue.messenger(); }

    virtual bool dispatch() = 0;

    void dispatch_cb(const XrlError& e)
    {
	if (e != XrlCmdError::OKAY()) {
	    XLOG_ERROR("Sent xrl got response %s\n", e.str().c_str());
	    queue().kill_messenger();
	    return ;
	}
	queue().crank();
    }

protected:
    FinderXrlCommandQueue& _queue;
};

#include "xrl/interfaces/finder_client_xif.hh"

/**
 * @short Send "hello" Xrl to Client.
 */
class FinderSendHelloToClient : public FinderXrlCommandBase {
public:
    FinderSendHelloToClient(FinderXrlCommandQueue& q,
			    const string&	   tgtname)
	: FinderXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    bool dispatch()
    {
	XrlFinderClientV0p2Client client(&(queue().messenger()));
	return client.send_hello(_tgtname.c_str(),
		  callback(static_cast<FinderXrlCommandBase*>(this),
			   &FinderXrlCommandBase::dispatch_cb));
    }

protected:
    string _tgtname;
};

/**
 * @short Send "remove xrl" to client.
 */
class FinderSendRemoveXrl : public FinderXrlCommandBase
{
public:
    FinderSendRemoveXrl(FinderXrlCommandQueue& q,
			const string&	       tgtname,
			const string&	       xrl)
	: FinderXrlCommandBase(q), _tgtname(tgtname), _xrl(xrl)
    {
    }

    ~FinderSendRemoveXrl()
    {
	_tgtname = _xrl = "croak";
    }

    bool dispatch()
    {
	XrlFinderClientV0p2Client client(&(queue().messenger()));
	return client.send_remove_xrl_from_cache(_tgtname.c_str(), _xrl,
		  callback(static_cast<FinderXrlCommandBase*>(this),
			   &FinderXrlCommandBase::dispatch_cb));
    }

protected:
    string _tgtname;
    string _xrl;
};

/**
 * @short Send "remove xrls for target" to client.
 */
class FinderSendRemoveXrls : public FinderXrlCommandBase
{
public:
    FinderSendRemoveXrls(FinderXrlCommandQueue& q,
			 const string&		tgtname)
	: FinderXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    ~FinderSendRemoveXrls()
    {
	_tgtname = "croak";
    }

    bool dispatch()
    {
	XrlFinderClientV0p2Client client(&(queue().messenger()));
	return client.send_remove_xrls_for_target_from_cache(
		     _tgtname.c_str(), _tgtname,
		     callback(static_cast<FinderXrlCommandBase*>(this),
			      &FinderXrlCommandBase::dispatch_cb));
    }

protected:
    string _tgtname;
};

/**
 * @short Send tunneled Xrl to client.  Client is expected to be
 * able to dispatch Xrl.
 */
class FinderSendTunneledXrl : public FinderXrlCommandBase
{
public:
    FinderSendTunneledXrl(FinderXrlCommandQueue& q,
			  const string&		 tgtname,
			  const string&		 xrl)
	: FinderXrlCommandBase(q), _tgtname(tgtname), _xrl(xrl)
    {
    }

    ~FinderSendTunneledXrl()
    {
	_tgtname = "croak";
    }

    void dispatch_cb(const XrlError& e,
		     const uint32_t* /* p_errno */,
		     const string*   /* p_errtxt */)
    {
	//	if (e != XrlError::OKAY())
	FinderXrlCommandBase::dispatch_cb(e);
	// TODO: XXX
    }

    bool dispatch()
    {
	XrlFinderClientV0p2Client client(&(queue().messenger()));
	return client.send_dispatch_tunneled_xrl(_tgtname.c_str(), _xrl,
		 callback(this, &FinderSendTunneledXrl::dispatch_cb));
    }

protected:
    string _tgtname;
    string _xrl;
};

#endif // __LIBXIPC_FINDER_XRL_QUEUE_HH__
